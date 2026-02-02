#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>

/* FSM */
size_t	Request::feed(const std::string& data)
{
	size_t	i = 0;
	while (i < data.length())
	{
		unsigned char ch = data[i];
		switch (_state)
		{
			case STATE_START:
			{
				if (ch == '\r' || ch == '\n') {
					++i;
					continue ;
				}
				_state = STATE_METHOD;
				break ;
			}
			case STATE_METHOD:
			{
				if (!std::isalpha(ch)) {
					_state = STATE_SPACE_BEFORE_URI;
					continue ;
					i++;
				}
				break ;
			}
			case STATE_SPACE_BEFORE_URI:
			{
				if (ch != ' ') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					// throw RequestException(400);
					_state = STATE_ERROR; continue ;
				}
				_method = _tmp_buffer;
				_tmp_buffer.clear();
				_state = STATE_URI_START;
				++i;
				continue ;
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
					_state = STATE_ERROR; continue ;
				}
				break;
			}
			case STATE_URI_SCHEME:
			{
				// Reading "http","https",...
				if (ch == ':') {
					_uri_scheme = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_SCHEME_FIRST_SLASH;
					++i;
					continue;
				}
				else if (!_is_scheme_char(ch)) {
					_state = STATE_ERROR; continue ;
				}
				break;
			}
			case STATE_SCHEME_FIRST_SLASH:
			{
				if (ch != '/') {
					_state = STATE_ERROR; continue ;
				}

				_state = STATE_SCHEME_SECOND_SLASH;
				i++;
				continue;
			}
			case STATE_SCHEME_SECOND_SLASH:
			{
				if (ch != '/') {
					_state = STATE_ERROR; continue ;
				}

				_state = STATE_URI_AUTH;
				i++;
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
					_state = STATE_ERROR; continue ;
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
					++i;
					continue;
				}
				else if (ch == '?')
				{
					_uri_path = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_QUERY;
					++i;
					continue;
				}
				else if (ch == '#')
				{
					_uri_path = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_FRAGMENT;
					++i;
					continue;
				}
				else if (!_is_pchar(ch) && ch != '/') {
					_state = STATE_ERROR; continue ;
				}
				break;
			}
			case STATE_URI_QUERY:
			{
				if (ch == ' ') {
					_uri_query = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_PROTOCOL_H;
					++i;
					continue ;
				}
				else if (ch == '#')
				{
					_uri_query = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_URI_FRAGMENT;
					++i;
					continue;
				}
				else if (!_is_query_char(ch)) {
					_state = STATE_ERROR; continue ;
				}
				break ;
			}
			case STATE_URI_FRAGMENT:
			{
				if (ch == ' ') {
					_state = STATE_PROTOCOL_H;
				}
				i++;
				continue;
			}
			case STATE_PROTOCOL_H:
			{
				if (ch != 'H') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_PROTOCOL_HT;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_HT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_PROTOCOL_HTT;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_HTT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_PROTOCOL_HTTP;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_HTTP:
			{
				if (ch != 'P') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_PROTOCOL_SLASH;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_SLASH:
			{
				if (ch != '/') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_MAJOR_DIGIT;
				++i;
				continue ;
			}
			case STATE_MAJOR_DIGIT:
			{
				if (ch != '1') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_DOT_AFTER_DIGIT;
				++i;
				continue ;
			}
			case STATE_DOT_AFTER_DIGIT:
			{
				if (ch != '.') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_MINOR_DIGIT;
				++i;
				continue ;
			}
			case STATE_MINOR_DIGIT:
			{
				if (ch != '1') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_FIRST_CR;
				++i;
				continue ;
			}
			case STATE_FIRST_CR:
			{
				if (ch != '\r') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_FIRST_LF;
				++i;
				continue ;
			}
			case STATE_FIRST_LF:
			{
				if (ch != '\n') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					_state = STATE_ERROR; continue ;
				}
				_state = STATE_FIELD_START;
				++i;
				continue ;
			}
			case STATE_FIELD_START:
			{
				if (ch == '\r') {
					_state = STATE_ERROR; continue ;
				}

				if (Request::_is_token(ch)) {
					ch = tolower(ch);
					_state = STATE_FIELD_NAME;
				}
				else {
					_state = STATE_ERROR; continue ;
				}
				break ;
			}
			case STATE_FIELD_NAME:
			{
				if (ch == '\r') {
					_state = STATE_FIELD_END;
					++i;
					continue ;
				}
				if (ch == ':') {// we have key:value
					_state = STATE_FIELD_VALUE;
				}
				else if (!Request::_is_token(ch)) {
					std::cerr << "Bad Header Key at: `" << ch << "`\n";
					_state = STATE_ERROR; continue ;
				}
				// make ch lowercase
				ch = tolower(ch);
				break ;
			}
			case STATE_FIELD_VALUE:
			{
				if (ch == '\r') {
					if (!_add_header()) continue ;
					_state = STATE_FIELD_LF;
					++i;
					continue ;
				}
				if (!_is_valid_value(ch)) {
					_state = STATE_ERROR; continue ;
				}
				break ;
			}
			case STATE_FIELD_LF:
			{
				if (ch != '\n') {
					_state = STATE_ERROR; continue ;
				}

				_state = STATE_FIELD_NAME;
				++i;
				continue ;
			}
			case STATE_FIELD_END:
			{
				if (ch != '\n') {
					_state = STATE_ERROR; continue ;
				}

				if (!__headers()) continue ;
				if (_body_exist) {
					if (_chunked_transfer) {
						_state = STATE_CHUNK_LENGTH;
					}
					else
						_state = STATE_BODY;
					++i;
					continue ;
				}
				else {
					_state = STATE_COMPLETE;
					++i;
					continue ;
				}
			}
			case STATE_BODY:
			{
				if (_body.size() < _content_length)
					_body += ch;
				if (_body.size() == _content_length)
					_state = STATE_COMPLETE;
				++i;
				continue ;
			}
			case STATE_CHUNK_LENGTH:
			{
				if (ch == '\r') {
					std::stringstream ss;
					ss << _tmp_buffer;
					_tmp_buffer.clear();
					ss >> std::hex >> _chunk_length;
					if (_chunk_length == 0 && _chunk_state == START) {
						_state = STATE_ERROR; continue ;
					}
					else if (_chunk_length == 0) {
						_state = STATE_COMPLETE;
						++i;
						continue ;
					}
					this->_chunk_state = PAYLOAD_BODY;
					this->_state = STATE_CHUNK_LF;
					++i;
					continue ;
				}
				else if (!isxdigit(ch)) {
					std::cerr << "bad chunk-length" << std::endl;
					_state = STATE_ERROR; continue ;
				}
				break ;
			}
			case STATE_CHUNK_FEED:
			{
				if (ch == '\r') {
					_state = STATE_CHUNK_LF;
					_chunk_state = LENGTH;
					++i;
					continue ;
				}

				if (_tmp_buffer.length() + 1 > _chunk_length) {
					_state = STATE_ERROR; continue ;
				}
				break ;
			}
			case STATE_CHUNK_CR:
			{
				if (ch != '\r') {
					_state = STATE_ERROR; continue ;
				}

				this->_state = STATE_CHUNK_LF;
				++i;
				continue ;
			}
			case STATE_CHUNK_LF:
			{
				if (ch != '\n') {
					_state = STATE_ERROR; continue ;
				}

				if (_chunk_state == PAYLOAD_BODY)
					_state = STATE_CHUNK_FEED;
				
				if (_chunk_state == LENGTH) {
					this->_body += _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_CHUNK_LENGTH;
				}
				++i;
				continue ;
			}
			case STATE_ERROR:
			{
				std::cerr << "Feed Parse Error\n";
				return i;
			}
			case STATE_COMPLETE:
			{
				return i;
			}
		}
		_tmp_buffer += ch;
		++i;
	}
	return i;
}

bool	Request::_add_header()
{
	size_t	delim = _tmp_buffer.find(':');
	std::string	key = _tmp_buffer.substr(0, delim);
	std::string value = _tmp_buffer.substr(delim + 1);

	if (key.empty()) {
		std::cerr << "key or value can't be empty!" << std::endl;
		_state = STATE_ERROR; return false;
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
			_state = STATE_ERROR; return false;
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
		_state = STATE_ERROR; return false;
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
			_state = STATE_ERROR; return false;
		}
		
		if (endptr == str || *endptr != '\0' || value.find('-') != std::string::npos) {
			std::cerr << "bad content-length" << std::endl;
			_state = STATE_ERROR; return false;
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

bool	Request::shouldKeepAlive() const
{
	return this->_keep_alive;
}

bool	Request::requestCompleted() const
{
	return _state == STATE_COMPLETE;
}

bool	Request::badRequest() const
{
	return _state == STATE_ERROR;
}

void	Request::resetState()
{
	_state = STATE_START;
	_m_index = 0;
	_content_length = 0;
	_chunked_transfer = false;
	_chunk_length = 0;
	_body_exist = false;
	_chunk_state = START;
	_keep_alive = true;
	_tmp_buffer.clear();
	_headers.clear();
}

Request::Request() :
	_state(STATE_START),
	_m_index(0),
	_content_length(0),
	_chunked_transfer(false),
	_chunk_length(0),
	_body_exist(false),
	_chunk_state(START),
	_keep_alive(true)
{}

Request::~Request() {}

std::string	Request::getMethod() const
{
	return _method;
}
std::string Request::getPath() const { return _uri_path; }
std::string	Request::getHeader(const std::string& key) const { return _headers.at(key); }
std::string	Request::getServerName() const { return _server_name; }
const std::string& Request::getBody() const
{
	return _body;
}

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"

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

	// 3. Body Metadata (The specific logic you asked for)
	std::cout << "\n"
			  << YELLOW << "[Body Metadata]" << RESET << "\n";
	if (_body_exist)
	{
		if (_chunked_transfer)
		{
			std::cout << "  Mode:         " << RED << "Chunked Transfer Encoding" << RESET << "\n";
			std::cout << "  Decoded Size: " << _body.length() << " bytes\n";
		}
		else
		{
			std::cout << "  Mode:         " << GREEN << "Content-Length" << RESET << "\n";
			std::cout << "  Header Value: " << _content_length << "\n";
			std::cout << "  Actual Read:  " << _body.length() << " bytes\n";

			// Safety Warning if they don't match
			if (_content_length != _body.length()) {
				std::cout << RED << "  WARNING: Body size mismatch!" << RESET << "\n";
			}
		}
	}
	else
	{
		std::cout << "  (No Body Present)\n";
	}

	// 4. The Actual Body Content
	// Only print if not empty, and maybe truncate if it's too huge
	if (_body_exist && !_body.empty())
	{
		std::cout << "\n"
				  << YELLOW << "[Body Content Preview]" << RESET << "\n";
		if (_body.length() > 1000)
		{
			std::cout << _body.substr(0, 1000) << "\n"
					  << RED << "...(truncated " << _body.length() - 1000 << " bytes)..." << RESET << "\n";
		}
		else
		{
			std::cout << _body << "\n";
		}
	}

	std::cout << CYAN << "===============================" << RESET << "\n"
			  << std::endl;
}