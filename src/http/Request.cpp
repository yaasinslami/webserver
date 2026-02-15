#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>
#include <cstdlib>

/* FSM */
ssize_t	Request::feed(const char* data, size_t size)
{
	size_t i = 0;
	for (; i < size; ++i)
	{
		unsigned char ch = data[i];
		switch (_state)
		{
			case STATE_START:
			{
				if (ch == '\r' || ch == '\n')
					continue ;
				_state = STATE_METHOD;
				break ;
			}
			case STATE_METHOD:
			{
				if (ch == ' ') {
					if (_tmp_buffer.empty())
						return -1;

					_method = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_START;
					continue;
				}
				if (!std::isalpha(ch)) {
					return -1;
				}
				break ;
			}
			case STATE_URI_START:
			{
				if (ch == '/') {
					_state = STATE_URI_PATH;
				}
				else if (_is_scheme_char(ch)) {
					_state = STATE_URI_SCHEME;
				}
				else {
					return -1;
				}
				break;
			}
			case STATE_URI_SCHEME:
			{
				if (ch == ':') {
					_uri_scheme = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_SCHEME_FIRST_SLASH;
					continue;
				}
				else if (!_is_scheme_char(ch)) {
					return -1;
				}
				break;
			}
			case STATE_SCHEME_FIRST_SLASH:
			{
				if (ch != '/') {
					return -1;
				}

				_state = STATE_SCHEME_SECOND_SLASH;
				continue;
			}
			case STATE_SCHEME_SECOND_SLASH:
			{
				if (ch != '/') {
					return -1;
				}
				_state = STATE_URI_AUTH;
				continue;
			}
			case STATE_URI_AUTH:
			{
				if (ch == '/' && !_tmp_buffer.empty()) {
					// we finish AUTH
					_uri_auth = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_PATH;
				}
				else if (!_is_unreserved(ch) && !_is_sub_delim(ch) && ch != ':' && ch != '@' && ch != '[' && ch != ']' && ch != '%') {
					return -1;
				}
				break ;
			}
			case STATE_URI_PATH:
			{
				if (ch == ' ')// URI FINISHED
				{
					_uri_path = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_PROTOCOL_H;
					continue;
				}
				else if (ch == '?')
				{
					_uri_path = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_QUERY;
					continue;
				}
				else if (ch == '#')
				{
					_uri_path = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_FRAGMENT;
					continue;
				}
				else if (!_is_pchar(ch) && ch != '/') {
					return -1;
				}
				break;
			}
			case STATE_URI_QUERY:
			{
				if (ch == ' ') {
					_uri_query = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_PROTOCOL_H;
					continue ;
				}
				else if (ch == '#')
				{
					_uri_query = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_FRAGMENT;
					continue;
				}
				else if (!_is_query_char(ch)) {
					return -1;
				}
				break ;
			}
			case STATE_URI_FRAGMENT:
			{
				if (ch == ' ') {
					_state = STATE_PROTOCOL_H;
				}
				continue;
			}
			case STATE_PROTOCOL_H:
			{
				if (ch != 'H') {
					return -1;
				}
				_state = STATE_PROTOCOL_HT;
				continue ;
			}
			case STATE_PROTOCOL_HT:
			{
				if (ch != 'T') {
					return -1;
				}
				_state = STATE_PROTOCOL_HTT;
				continue ;
			}
			case STATE_PROTOCOL_HTT:
			{
				if (ch != 'T') {
					return -1;
				}
				_state = STATE_PROTOCOL_HTTP;
				continue ;
			}
			case STATE_PROTOCOL_HTTP:
			{
				if (ch != 'P') {
					return -1;
				}
				_state = STATE_PROTOCOL_SLASH;
				continue ;
			}
			case STATE_PROTOCOL_SLASH:
			{
				if (ch != '/') {
					return -1;
				}
				_state = STATE_MAJOR_DIGIT;
				continue ;
			}
			case STATE_MAJOR_DIGIT:
			{
				if (ch != '1') {
					return -1;
				}
				_state = STATE_DOT_AFTER_DIGIT;
				continue ;
			}
			case STATE_DOT_AFTER_DIGIT:
			{
				if (ch != '.') {
					return -1;
				}
				_state = STATE_MINOR_DIGIT;
				continue ;
			}
			case STATE_MINOR_DIGIT:
			{
				if (ch != '1') {
					return -1;
				}
				_state = STATE_FIRST_CR;
				continue ;
			}
			case STATE_FIRST_CR:
			{
				if (ch != '\r') {
					return -1;
				}
				_state = STATE_FIRST_LF;
				continue ;
			}
			case STATE_FIRST_LF:
			{
				if (ch != '\n') {
					return -1;
				}
				_state = STATE_FIELD_START;
				continue ;
			}
			case STATE_FIELD_START:
			{
				if (ch == '\r') {
					return -1;
				}

				if (Request::_is_token(ch)) {
					ch = tolower(ch);
					_state = STATE_FIELD_NAME;
				}
				else {
					return -1;
				}
				break ;
			}
			case STATE_FIELD_NAME:
			{
				if (ch == '\r') {
					_state = STATE_FIELD_END;

					continue ;
				}
				if (ch == ':') {// we have key:value
					_state = STATE_FIELD_VALUE;
				}
				else if (!Request::_is_token(ch)) {
					std::cerr << "Bad Header Key at: `" << ch << "`\n";
					return -1;
				}
				// make ch lowercase
				ch = tolower(ch);
				break ;
			}
			case STATE_FIELD_VALUE:
			{
				if (ch == '\r') {
					if (!_add_header()) continue ;
					_tmp_buffer.clear();
					_state = STATE_FIELD_LF;

					continue ;
				}
				if (!_is_valid_value(ch)) {
					return -1;
				}
				break ;
			}
			case STATE_FIELD_LF:
			{
				if (ch != '\n') {
					return -1;
				}

				_state = STATE_FIELD_NAME;
				continue ;
			}
			case STATE_FIELD_END:
			{
				if (ch != '\n') {
					return -1;
				}

				if (!__headers()) return -1;

				// here means we finish headers
				_headers_finished = true;
				// set headers_completed flag or call a helper function here that search for the correct config

				if (_body_exist) {
					// _body_file_path = "/tmp/webserv_req_" + _generate_random_id();
					_body_file_path = "/tmp/webserv_req";

					_body_file.open(_body_file_path.c_str(), std::ios::binary);
					if (!_body_file.is_open()) {
						std::cerr << "Error: Could not open temp file\n";
						return -1;
					}

					if (_chunked_transfer) {
						_state = STATE_CHUNK_LENGTH;
					}
					else {
						_state = STATE_BODY;
					}
				}
				else {
					_state = STATE_COMPLETE;
					return i + 1;
				}
				continue ;
			}
			case STATE_BODY:
			{
				size_t  available_in_buffer = size - i;
				size_t  needed_for_body     = _content_length - _total_body_bytes;

				size_t  bytes_to_write      = std::min(available_in_buffer, needed_for_body);

				if (_body_file.is_open()) {// before writing - checking b3da wach mefto7
					_body_file.write(data + i, bytes_to_write);
				}

				_total_body_bytes += bytes_to_write;
				if (bytes_to_write > 0)
					i += bytes_to_write - 1;

				if (_total_body_bytes >= _content_length) {
					_body_file.close();
					_state = STATE_COMPLETE;
					return i + 1;
				}
				continue ;
			}
			case STATE_CHUNK_LENGTH:
			{
				if (ch == '\r') {
					std::stringstream ss;
					ss << _tmp_buffer;
					_tmp_buffer.clear();
					ss >> std::hex >> _chunk_length;

					_state = STATE_CHUNK_LF;

					if (_chunk_length == 0) {
						_chunk_state = CHUNK_END;
						continue ;
					}					
					_chunk_bytes_written = 0;
					_chunk_state = PAYLOAD_BODY;
					continue ;
				}
				else if (!isxdigit(ch)) {
					std::cerr << RED << "Bad chunk-length" << RESET << std::endl; return -1 ;
				}
				break ;
			}
			case STATE_CHUNK_CR:
			{
				if (ch != '\r') {
					std::cerr << "Chunking Error: Expected CR after data, got: " << (int)ch << "\n";
					return -1;
				}

				_chunk_state = LENGTH;
				_state = STATE_CHUNK_LF;
				continue ;
			}
			case STATE_CHUNK_LF:
			{
				if (ch != '\n') {
					std::cerr << "Chunking Error: Expected LF, got: " << (int)ch << "\n";
					return -1;
				}
				// switcher, switches between Length and body chunk
				if (_chunk_state == PAYLOAD_BODY)
					_state = STATE_CHUNK_DATA;
				else if (_chunk_state == LENGTH)
					_state = STATE_CHUNK_LENGTH;
				else if (_chunk_state == CHUNK_END) {
					_state = STATE_CHUNK_FINAL_CR;
				}
				continue ;
			}
			case STATE_CHUNK_FINAL_CR:
			{
				if (ch != '\r')
					return -1; // Expected final empty line
				_state = STATE_CHUNK_FINAL_LF;
				continue;
			}
			case STATE_CHUNK_FINAL_LF:
			{
				if (ch != '\n') return -1;

				if (_body_file.is_open()) {
					_body_file.close();
				}
				_state = STATE_COMPLETE;
				return i + 1;
			}
			case STATE_CHUNK_DATA:
			{
				size_t  available_in_buffer = size - i;				
				size_t  needed_for_chunk    = _chunk_length - _chunk_bytes_written;

				size_t  bytes_to_write      = std::min(available_in_buffer, needed_for_chunk);

				if (_body_file.is_open()) {
					_body_file.write(data + i, bytes_to_write);
				}

				_chunk_bytes_written += bytes_to_write;
				_total_body_bytes += bytes_to_write;
				if (bytes_to_write > 0)
					i += bytes_to_write - 1;

				if (_chunk_bytes_written >= _chunk_length) {
					_state = STATE_CHUNK_CR;
				}
				continue ;
			}
			case STATE_COMPLETE:
			{
				std::cout << GREEN << "✅\t---> Request Completed" << RESET << std::endl;
				return i;
			}
		}
		if (_state < STATE_BODY) {
			_tmp_buffer += ch;
		}
	}

	(void)_chunk_state;
	return i;
}

bool	Request::_add_header()
{
	size_t	delim = _tmp_buffer.find(':');
	std::string	key = _tmp_buffer.substr(0, delim);
	std::string value = _tmp_buffer.substr(delim + 1);

	if (key.empty()) {
		std::cerr << "key or value can't be empty!" << std::endl;
		return false;
	}

	const std::string whitespace = " \t";
	size_t start = value.find_first_not_of(whitespace);
	size_t end = value.find_last_not_of(whitespace);

	if (start == std::string::npos)
		value = "";
	else {
		value = value.substr(start, end - start + 1);
	}

	if (_headers.count(key)) {
		if (key == "host" || key == "content-length") {
			std::cerr << "Duplicated Header Not Allowed!\n";
			return false;
		}

		_headers[key] += ", " + value;
	}
	else
		_headers[key] = value;

	_tmp_buffer.clear();

	return true;
}

bool	Request::__headers()
{
	if (_headers.find("host") == _headers.end()) {
		std::cerr << "Error: HTTP/1.1 request missing 'Host' header\n";
		return false;
	}

	std::string	raw_host;
	if (!_uri_auth.empty()) {
		raw_host = _uri_path;
	}
	else {
		raw_host = _headers["host"];
	}

	size_t colon_pos = raw_host.find(':');

	if (colon_pos != std::string::npos) {
		_server_name = raw_host.substr(0, colon_pos);
	}
	else {
		_server_name = raw_host;
	}

	if (_headers.count("content-length")) {
		std::string	value = _headers["content-length"];
	
		char	*endptr;
		const char* str = value.c_str();

		errno = 0;
	
		unsigned long len = std::strtoul(str, &endptr, 10);
		if (errno == ERANGE) {
			std::cerr << "content-length too large!";
			return false;
		}
		
		if (endptr == str || *endptr != '\0' || value.find('-') != std::string::npos) {
			std::cerr << "bad content-length" << std::endl;
			return false;
		}

		_content_length = (size_t)len;
		_body_exist = true;
	}

	if (_headers.count("transfer-encoding"))
	{
		if (_headers["transfer-encoding"].find("chunked") != std::string::npos) {
			_chunked_transfer = true;
		}
		_body_exist = true;
	}


	if (_headers.count("connection") && _headers["connection"].find("close")) {
		std::cout << "--++---\texplicitly the client want to close the conection\n";
		_keep_alive = false;
	}

	// Content-Type Header Still ...
	std::cout << "we finish parsing of headers\n";
	return true;
}

bool Request::_is_scheme_char(unsigned char c)
{
	return (isalnum(c) || c == '+' || c == '-' || c == '.');
}

bool Request::_is_unreserved(unsigned char c)
{
	// ALPHA / DIGIT / "-" / "." / "_" / "~"
	return (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~');
}

bool Request::_is_sub_delim(unsigned char c)
{
	std::string sub_delims = "!$&'()*+,;=";
	return (sub_delims.find(c) != std::string::npos);
}

bool Request::_is_pchar(unsigned char c)
{
	return (_is_unreserved(c) || _is_sub_delim(c) || c == ':' || c == '@' || c == '%');
}

bool Request::_is_query_char(unsigned char c)
{
	return (_is_pchar(c) || c == '/' || c == '?');
}

bool Request::_is_token(unsigned char c)
{
	if (isalnum(c))
		return true;

	std::string symbols = "!#$%&'*+-.^_`|~";
	return (symbols.find(c) != std::string::npos);
}

bool Request::_is_valid_value(unsigned char c)
{
	if (c == ' ' || c == '\t')
		return true;

	// Allow Visible ASCII (0x21 - 0x7E)
	if (c >= 0x21 && c <= 0x7E)
		return true;

	// Allow Extended ASCII (obs-text) (0x80 - 0xFF)
	if (c >= 0x80)
		return true;

	return false;
}

bool	Request::shouldKeepAlive() const { return this->_keep_alive; }

bool	Request::requestCompleted()
{
	if (_state == STATE_COMPLETE) {
		// resetState();
		return true;
	}
	return false;
}

bool	Request::headersFinished() const { return _headers_finished; }

size_t	Request::getContentLength() const { return _content_length; }

void	Request::resetState()
{
	_state = STATE_START;
	_content_length = 0;
	_chunk_length = 0;
	_chunk_bytes_written = 0;
	_total_body_bytes = 0;
	_chunked_transfer = false;
	_body_exist = false;
	_chunk_state = LENGTH;
	_keep_alive = true;
	_tmp_buffer.clear();
	_headers.clear();

	if (_body_file.is_open()) {
		_body_file.close();
	}
	_headers_finished = false;
	_is_cgi = false;
}

Request::Request() :
	_state(STATE_START),
	_content_length(0),
	_chunked_transfer(false),
	_chunk_length(0),
	_chunk_bytes_written(0),
	_total_body_bytes(0),
	_body_exist(false),
	_chunk_state(LENGTH),
	_keep_alive(true),
	_headers_finished(false),
	_is_cgi(false)
{}

Request::~Request() {}

Request::Request(const Request& other) { *this = other; }

Request& Request::operator=(const Request& other)
{
	_state = other._state;
	_tmp_buffer = other._tmp_buffer;
	_content_length = other._content_length;
	_chunked_transfer = other._chunked_transfer;
	_chunk_length = other._chunk_length;
	_method = other._method;
	_uri_scheme = other._uri_scheme;
	_uri_auth = other._uri_auth;
	_uri_path = other._uri_path;
	_uri_query = other._uri_query;
	_server_name = other._server_name;
	_headers = other._headers;
	_body_file_path = other._body_file_path;
	_chunk_bytes_written = other._chunk_bytes_written;
	_total_body_bytes = other._total_body_bytes;
	_body_exist = other._body_exist;
	_chunk_state = other._chunk_state;
	_keep_alive = other._keep_alive;
	_headers_finished = other._headers_finished;
	_is_cgi = other._is_cgi;

	return (*this);
}

std::string	Request::getMethod() const { return _method; }
std::string Request::getPath() const { return _uri_path; }
std::string	Request::getHeader(const std::string& key) const { return _headers.at(key); }

// we will replace this with the actual body from the file
// std::string Request::getBody() const { return "Hello world"; }
std::string Request::getBody() const { return std::string(1024 * 100, 'A'); }

std::string	Request::getServerName() const { return _server_name; }
size_t		Request::getBodySize() const { return _total_body_bytes; }

bool		Request::isCgi() const { return _is_cgi; }
void		Request::setIsCgi(bool state) { _is_cgi = state; }

std::map<std::string, std::string> Request::getHeaders() const { return _headers; }

void Request::show() const
{
	std::cout << "\n"
			  << CYAN << "=== HTTP REQUEST DEBUG INFO ===" << RESET << std::endl;

	// 1. Request Line Info
	std::cout << YELLOW << "[Request Line]" << RESET << "\n";
	std::cout << "  Method:       " << GREEN << _method << RESET << "\n";
	std::cout << "  Sheme:         " << _uri_scheme << "\n";
	std::cout << "  Auth:         " << _uri_auth << "\n";
	std::cout << "  Path:         " << _uri_path << "\n";
	std::cout << "  Query:         " << _uri_query << "\n";
	// std::cout << "  Query:        " << (_query.empty() ? "(empty)" : _query) << "\n";
	std::cout << "  Target Host:  " << (_server_name.empty() ? "(default)" : _server_name) << "\n";

	// 2. Headers
	std::cout << "\n"
			  << YELLOW << "[Headers]" << RESET << "\n";
	if (_headers.empty())
	{
		std::cout << "  (No Headers)\n";
	}
	else
	{
		std::map<std::string, std::string>::const_iterator it;
		for (it = _headers.begin(); it != _headers.end(); ++it) {
			std::cout << "  " << BLUE << it->first << RESET << ": " << it->second << "\n";
		}
	}

	// Body
	std::cout << "\n"
			  << YELLOW << "[Body Metadata]" << RESET << "\n";
	if (_body_exist) {
		std::cout << "Body Eixst\n";
	}
	else
		std::cout << "  (No Body Present)\n";

	std::cout << CYAN << "===============================" << RESET << "\n"
			  << std::endl;
}

RequestState Request::getState() const { return _state; }