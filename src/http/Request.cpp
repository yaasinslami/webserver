#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <cerrno>
#include <cstring>

/* Feed; Where the Parsing Magic comes true */
void	Request::feed(const char* data, size_t size)
{
	for (size_t i = 0; i < size; ++i) {
		unsigned char ch = data[i];
		switch (_state)
		{
			case STATE_METHOD:
			{
				if (ch == ' ') {
					if (_method.empty()) { _state = STATE_ERROR; _code = 400; }
					else _state = STATE_URI_START;
				}
				else if (!std::isalpha(ch)) { _state = STATE_ERROR; _code = 400; }
				else if (_method.length() >= MAX_METHOD_LEN) { _state = STATE_ERROR; _code = 414; }
				else _method += ch;
				break ;
			}
			case STATE_URI_START:
			{
				if (ch == '/') { _uri_path += ch; _state = STATE_URI_PATH; }
				else { _state = STATE_ERROR; _code = 400; }
				break;
			}
			case STATE_URI_PATH:
			{
				if (ch == ' ') { _state = STATE_PROTOCOL; }
				else if (ch == '?') { _state = STATE_URI_QUERY; }
				else if (ch == '#') { _state = STATE_URI_FRAGMENT; }
				else if (!_is_pchar(ch) && ch != '/') { _state = STATE_ERROR; _code = 400; }
				else if ( _uri_path.length() >= MAX_URI_LEN) { _state = STATE_ERROR; _code = 414; }
				else _uri_path += ch;
				break ;
			}
			case STATE_URI_QUERY:
			{
				if (ch == ' ') { _state = STATE_PROTOCOL; }
				else if (ch == '#') { _state = STATE_URI_FRAGMENT; }
				else if (!_is_query_char(ch)) { _state = STATE_ERROR; _code = 400; }
				else if ((_uri_path.length() + _uri_query.length()) >= MAX_URI_LEN) { _state = STATE_ERROR; _code = 414; }
				else _uri_query += ch;
				break ;
			}
			case STATE_URI_FRAGMENT:
			{
				if (ch == ' ') _state = STATE_PROTOCOL;
				break ;
			}
			case STATE_PROTOCOL:
			{
				if (ch == '\r') {
					if (_protocol != "HTTP/1.1") { _state = STATE_ERROR; _code = 400; }
					else _state = STATE_PROTOCOL_LF;
					break ;
				}
				else if (_protocol.length() > 8) { _state = STATE_ERROR; _code = 414; }
				else _protocol += ch;
				break ;
			}
			case STATE_PROTOCOL_LF:
			{
				if (ch != '\n') { _state = STATE_ERROR; _code = 400; }
				else _state = STATE_FIELD_KEY;
				break ;
			}
			case STATE_FIELD_KEY:
			{
				if (_tmp_key.empty() && ch == '\r') _state = STATE_HEADERS_DONE;
				else if (!_tmp_key.empty() && ch == ':') _state = STATE_FIELD_VALUE;
				else if (!Request::_is_token(ch)) { _state = STATE_ERROR; _code = 400; }
				else if (_tmp_key.length() >= MAX_HEADER_KEY_LEN) { _state = STATE_ERROR; _code = 431; }
				else _tmp_key += std::tolower(ch);
				break ;
			}
			case STATE_FIELD_VALUE:
			{
				if (ch == '\r') _add_header();
				else if (_tmp_value.empty() && (ch == ' ' || ch == '\t')) continue ;
				else if (!_is_valid_value(ch)) { _state = STATE_ERROR; _code = 400; }
				else if (_tmp_value.length() >= MAX_HEADER_VALUE_LEN) { _state = STATE_ERROR; _code = 431; }
				else _tmp_value += ch;
				break;
			}
			case STATE_FIELD_LF:
			{
				if (ch != '\n') { _state = STATE_ERROR; _code = 400; }
				else _state = STATE_FIELD_KEY;
				break ;
			}
			case STATE_HEADERS_DONE:
			{
				if (ch != '\n') { _state = STATE_ERROR;  _code = 400; break; }
				_validate_headers();
				if (_state == STATE_ERROR) break;
				if (_body_exist) {
					_body_file_path = "/tmp/webserv_req_" + _generate_random_id();
					_body_file.open(_body_file_path.c_str(), std::ios::binary | std::ios::trunc);
					if (!_body_file.is_open()) { _state = STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break ; }
					_state = (_chunked_transfer) ? STATE_CHUNKED : STATE_BODY;
				}
				else _state = STATE_COMPLETE;
				break ;
			}
			case STATE_BODY:
			{
				if (!_body_file.is_open()) {_state = STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break; }
				size_t available = size - i;
				size_t needed = _content_length - _total_body_bytes;
				size_t to_write = std::min(available, needed);
				if (to_write > 0) {
					_body_file.write(data + i, to_write);
					if (_body_file.fail()) {_state = STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break; }
					_total_body_bytes += to_write;
					i += to_write - 1;
				}
				if (_total_body_bytes >= _content_length) { _body_file.close(); _state = STATE_COMPLETE; }
				break;
			}
			case STATE_CHUNKED:
			{
				_chunked_body(data, size, i);
				break ;
			}
			case STATE_COMPLETE:
			{
				if (_body_file.is_open()) 
					_body_file.close();
				return ;
			}
			case STATE_ERROR:
			{
				if (_code >= 500) {
					std::cerr << RED << "[Fatal] Internal Server Error: "
							  << _sys_error_msg << RESET << std::endl;
				}
				else std::cerr << YELLOW << "[Warn] Bad Request (" << _code << ")" << RESET << std::endl;
				return;
			}
		}
	}
}
/* Headers stuff */
void	Request::_add_header()
{
	size_t end = _tmp_value.find_last_not_of(" \t");
	if (end != std::string::npos) _tmp_value = _tmp_value.substr(0, end + 1);
	if (_headers.size() >= MAX_HEADERS_COUNT) { _state = STATE_ERROR; _code = 431; return; }
	if (_headers.count(_tmp_key)) {
		if (_tmp_key == "host" || _tmp_key == "content-length" || _tmp_key == "transfer-encoding") { _state = STATE_ERROR; _code = 400;return; }
		size_t new_len = _headers[_tmp_key].length() + 2 + _tmp_value.length();
		if (new_len > MAX_HEADER_VALUE_LEN) { _state = STATE_ERROR; _code = 431; return; }
		_headers[_tmp_key] += ", " + _tmp_value;
	}
	else _headers[_tmp_key] = _tmp_value;
	_tmp_key.clear(); _tmp_value.clear();
	_state = STATE_FIELD_LF;
}
void	Request::_validate_headers()
{
	if (_headers.find("host") == _headers.end()) { _state = STATE_ERROR; _code = 400; return ;}
	if (_headers.count("content-length")) {
		char	*endptr;
		const char* str = _headers["content-length"].c_str();
		errno = 0;
		unsigned long len = std::strtoul(str, &endptr, 10);
		if (errno == ERANGE || endptr == str || *endptr != '\0') { _state = STATE_ERROR; _code = 400; return; }
		_content_length = (size_t)len;
		_body_exist = _content_length == 0 ? false : true;
	}
	if (_headers.count("transfer-encoding")) {
		// if (_body_exist) {_state = STATE_ERROR; _code = 400;return; }// Transfer-Encoding overrides Content-Length
		if (_headers["transfer-encoding"].find("chunked") != std::string::npos) {
			_chunked_transfer = true;
			_body_exist = true;
			std::cout << "DEBUG: Chunked transfer detected!" << std::endl;
		}
		else { _state = STATE_ERROR; _code = 501; return; }// 501 Not Implemented
	}
	if (_headers.count("connection") && _headers["connection"].find("close") != std::string::npos) _keep_alive = false;
}
/* Getters */
const std::string&	Request::getMethod() const { return _method; }
const std::string& Request::getPath() const { return _uri_path; }
const std::string& Request::getQuery() const { return _uri_query; }
std::map<std::string, std::string> Request::getHeaders() const { return _headers; }
const std::string&	Request::getHeader(const std::string& key) const { return _headers.at(key); }
const size_t&		Request::getBodySize() const { return _total_body_bytes; }
const RequestState& Request::getState() const { return _state; }
const size_t&	Request::getContentLength() const { return _content_length; }
const std::string& Request::getBodyPath() const { return _body_file_path; }
bool	Request::shouldKeepAlive() const { return _keep_alive == true; }
bool		Request::getIsCgi() const { return _is_cgi; }
void		Request::setIsCgi(bool state) { _is_cgi = state; }
const short& Request::getCode() const { return _code; }
bool Request::hasBody() const { return _body_exist; }
/* Random Functions */
void	Request::setState(RequestState state) { _state = state; }
void Request::_chunked_body(const char* data, size_t size, size_t& i)
{
	unsigned char ch = data[i];
	switch (_chunk_state) {
		case LENGTH:
		{
			if (std::isxdigit(ch)) {
				size_t digit = (ch >= '0' && ch <= '9') ? (ch - '0') : (std::tolower(ch) - 'a' + 10);
				if (_chunk_length > (std::numeric_limits<size_t>::max() >> 4)) { _state = STATE_ERROR; _code = 400; break ; }
				_chunk_length = (_chunk_length * 16) + digit;
			}
			else if (ch == '\r') _chunk_state = LENGTH_LF;
			else if (ch == ';') _chunk_state = LENGTH_EXTENSION;
			else { _state = STATE_ERROR; _code = 400; }
			break ;
		}
		case LENGTH_EXTENSION:
		{
			if (ch == '\r') _chunk_state = LENGTH_LF;;
			break ;
		}
		case LENGTH_LF:
		{
			if (ch == '\n') {
				if (_chunk_length == 0) { _body_file.close(); _chunk_state = PAYLOAD_BODY_CR; }
				else { _chunk_state = PAYLOAD_BODY; _chunk_bytes_read = 0; }
			}
			else { _state = STATE_ERROR; _code = 400; }
			break ;
		}
		case PAYLOAD_BODY:
		{
			if (!_body_file.is_open()) { _state = STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break; }
			size_t needed = _chunk_length - _chunk_bytes_read;
			size_t available = size - i;
			size_t to_write = std::min(needed, available);
			if (to_write > 0) {
				_body_file.write(data + i, to_write);
				if (_body_file.fail()) { _state = STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break; }
				_chunk_bytes_read += to_write;
				_total_body_bytes += to_write;
				i += to_write - 1;
			}
			if (_chunk_bytes_read >= _chunk_length) _chunk_state = PAYLOAD_BODY_CR;
			break;
		}
		case PAYLOAD_BODY_CR:
		{
			if (ch == '\r') _chunk_state = PAYLOAD_BODY_LF;
			else { _state = STATE_ERROR; _code = 400; }
			break ;
		}
		case PAYLOAD_BODY_LF:
		{
			if (ch == '\n' && _chunk_length != 0) { _chunk_state = LENGTH; _chunk_length = 0; }
			else if (_chunk_length == 0) _state = STATE_COMPLETE;
			else { _state = STATE_ERROR; _code = 400; }
			break ;
		}
	}
}
bool	Request::requestCompleted() { return _state == STATE_COMPLETE; }
void	Request::resetState()
{
	/* strings */
	_method.clear();
	_uri_path.clear();
	_uri_query.clear();
	_protocol.clear();
	_tmp_key.clear();
	_tmp_value.clear();
	_body_file_path.clear();
	_sys_error_msg.clear();
	/* headers map */
	_headers.clear();
	/* states */
	_state = STATE_METHOD;
	_chunk_state = LENGTH;
	_content_length = 0;
	_chunk_length = 0;
	_chunk_bytes_read = 0;
	_total_body_bytes = 0;
	_body_exist = false;
	_chunked_transfer = false;
	_keep_alive = true;
	/* body file - Reset the stream's internal error state flags */
	if (_body_file.is_open())
		_body_file.close();
	_body_file.clear();
	_is_cgi = false;
}

std::string Request::_generate_random_id()
{
	std::stringstream ss;
	static int counter = 0;
	ss << std::time(NULL) << "_" << counter++;
	return ss.str();
}
/* RFC validation */
bool Request::_is_unreserved(unsigned char c) { return (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~'); }
bool Request::_is_sub_delim(unsigned char c)
{
	std::string sub_delims = "!$&'()*+,;=";
	return (sub_delims.find(c) != std::string::npos);
}
bool Request::_is_pchar(unsigned char c) { return (_is_unreserved(c) || _is_sub_delim(c) || c == ':' || c == '@' || c == '%'); }
bool Request::_is_query_char(unsigned char c) { return (_is_pchar(c) || c == '/' || c == '?'); }
bool Request::_is_token(unsigned char c)
{
	if (isalnum(c)) return true;
	std::string symbols = "!#$%&'*+-.^_`|~";
	return (symbols.find(c) != std::string::npos);
}
bool Request::_is_valid_value(unsigned char c)
{
	if ((c >= 0x20 && c <= 0x7E) || c == 0x9 || c >= 0x80) return true;
	return false;
}
/* Request Log */
void Request::log() const
{
	std::cout << "\n" << CYAN << "=== HTTP REQUEST DEBUG INFO ===" << RESET << std::endl;
	// Request Line Info
	std::cout << YELLOW << "[Request Line]" << RESET << "\n";
	std::cout << "  Method:       " << GREEN << getMethod() << RESET << "\n";
	std::cout << "  Path:         " << getPath() << "\n";
	std::cout << "  Query:         " << getQuery() << "\n";
	// Headers
	std::cout << "\n" << YELLOW << "[Headers]" << RESET << "\n";
	if (_headers.empty()) std::cout << "  (No Headers)\n";
	else {
		std::map<std::string, std::string>::const_iterator it;
		for (it = _headers.begin(); it != _headers.end(); ++it)
			std::cout << "  " << BLUE << it->first << RESET << ": " << it->second << "\n";
	}
	// Body
	std::cout << "\n" << YELLOW << "---- [Body] ----" << RESET << "\n";
	if (_body_exist) {
		std::cout << "Body Eixst\t";
		std::cout << BLUE << getBodyPath() << RESET << std::endl;;
	}
	else
		std::cout << "  (No Body Present)\n";
	std::cout << CYAN << "===============================" << RESET << "\n\n";
}
/* Constructors */
Request::Request(short& code) :
	_state(STATE_METHOD),
	_code(code),
	_body_exist(false),
	_chunked_transfer(false),
	_keep_alive(true),
	_content_length(0),
	_total_body_bytes(0),
	_chunk_length(0),
	_chunk_bytes_read(0),
	_chunk_state(LENGTH),
	_is_cgi(false)
{}
Request::~Request()
{
	if (_body_file.is_open())
		_body_file.close();
	_body_file.clear();
	if (_state != STATE_COMPLETE && !_body_file_path.empty())
		std::remove(_body_file_path.c_str());
}
Request::Request(const Request& other) : _code(other._code) { *this = other; }
Request& Request::operator=(const Request& other)
{
	if (this != &other) {
		_state = other._state;
		_code = other._code;
		_content_length = other._content_length;
		_chunked_transfer = other._chunked_transfer;
		_chunk_length = other._chunk_length;
		_method = other._method;
		_uri_path = other._uri_path;
		_uri_query = other._uri_query;
		_headers = other._headers;
		_body_file_path = other._body_file_path;
		_chunk_bytes_read = other._chunk_bytes_read;
		_total_body_bytes = other._total_body_bytes;
		_body_exist = other._body_exist;
		_chunk_state = other._chunk_state;
		_keep_alive = other._keep_alive;
		_is_cgi = other._is_cgi;
	}
	return (*this);
}
