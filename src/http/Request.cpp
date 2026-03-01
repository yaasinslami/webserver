#include "Request.hpp"
#include "Utils.hpp"
#include <sstream>
#include <iostream>
#include <limits>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>

/* Feed; Where the Parsing Magic comes true */
size_t	Request::feed(const char* data, size_t size)
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
				if ( _is_pchar(ch) || ch == '/') { _uri_path += ch; _state = STATE_URI_PATH; }
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
				else { _state = STATE_FIELD_KEY; _parseUriPath(); }
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
				return i + 1;
			}
			case STATE_BODY:
			{
				size_t available = size - i;
				size_t needed = _content_length - _total_body_bytes;
				size_t to_write = std::min(available, needed);
				if (to_write > 0) {
					ssize_t bytes_written = write(_body_fd, data + i, to_write);
					if (bytes_written <= 0) { _state =  STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break; }
					_total_body_bytes += bytes_written;
					i += bytes_written - 1;
				}
				if (_total_body_bytes >= _content_length) { close(_body_fd); _state = STATE_COMPLETE; return i + 1; }
				break;
			}
			case STATE_CHUNKED:
			{
				_chunked_body(data, size, i);
				if (_state == STATE_ERROR || _state == STATE_COMPLETE) return i + 1;
				break ;
			}
			case STATE_COMPLETE:
			{}
			case STATE_ERROR:
			{}
		}
	}
	return size;
}
/* Headers stuff */
void	Request::_add_header()
{
	size_t end = _tmp_value.find_last_not_of(" \t");
	if (end != std::string::npos) _tmp_value = _tmp_value.substr(0, end + 1);
	if (_headers.size() >= MAX_HEADERS_COUNT) { _state = STATE_ERROR; _code = 431; return; }
	if (_headers.count(_tmp_key)) {
		if (_tmp_key == "host" || _tmp_key == "content-length") { _state = STATE_ERROR; _code = 400;return; }
		size_t new_len = _headers[_tmp_key].length() + 2 + _tmp_value.length();
		if (new_len > MAX_HEADER_VALUE_LEN) { _state = STATE_ERROR; _code = 431; return; }
		_headers[_tmp_key] += ", " + _tmp_value;
	}
	else _headers[_tmp_key] = _tmp_value;
	_tmp_key.clear(); _tmp_value.clear();
	_state = STATE_FIELD_LF;
}
bool	Request::validateHeaders()
{
	if (_headers.find("host") == _headers.end()) { _code = 400; return false;}

	if (_headers.count("content-length")) {
		char	*endptr;
		const char* str = _headers["content-length"].c_str();
		errno = 0;
		unsigned long len = std::strtoul(str, &endptr, 10);
		if (errno == ERANGE || endptr == str || *endptr != '\0') { _code = 400; return false; }
		_content_length = (size_t)len;
		_body_exist = (_content_length > 0);
		_has_content_length = true;
	}

	if (_headers.count("transfer-encoding")) {
		if (_has_content_length) { _code = 400; return false; }
		if (_headers["transfer-encoding"] == "chunked") {
			_chunked_transfer = true;
			_body_exist = true;
			std::cout << "DEBUG: Chunked transfer detected!" << std::endl;
		}
		else { _code = 501; return false; }// 501 Not Implemented
	}

	if (_method == "POST" && !_has_content_length && !_chunked_transfer) {
		_code = 411; // Length Required
		return false;
	}

	if (_headers.count("connection") && _headers["connection"].find("close") != std::string::npos) _keep_alive = false;
	
	return true;
}
/* Getters */
const std::string&	Request::getMethod() const { return _method; }
bool	Request::isBodyMethod(const std::string& method) const { return (method == "POST" ); }
const std::string& Request::getPath() const { return _uri_path; }
const std::string& Request::getQuery() const { return _uri_query; }
std::map<std::string, std::string> Request::getHeaders() const { return _headers; }
const std::string&	Request::getHeader(const std::string& key) const
{
	static const std::string empty_string = ""; 

	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end()) {
		return it->second;
	}
	return empty_string;
}
const size_t&		Request::getBodySize() const { return _total_body_bytes; }
const RequestState& Request::getState() const { return _state; }
const size_t&	Request::getContentLength() const { return _content_length; }
const std::string& Request::getBodyPath() const { return _body_file_path; }
const std::string& Request::getSysErrorMsg() const { return _sys_error_msg; }
bool	Request::shouldKeepAlive() const { return _keep_alive == true; }
bool		Request::getIsCgi() const { return _is_cgi; }
void		Request::setIsCgi(bool state) { _is_cgi = state; }
const short& Request::getCode() const { return _code; }
bool Request::hasBody() const { return _body_exist; }
bool Request::isChunked() const { return _chunked_transfer; }
std::string Request::getUploadFilename() const
{
	std::string disp = getHeader("content-disposition");
	if (!disp.empty()) {
		size_t filename_pos = disp.find("filename=\"");
		if (filename_pos != std::string::npos) {
			size_t start = filename_pos + 10;
			size_t end = disp.find("\"", start);
			if (end != std::string::npos) {
				return disp.substr(start, end - start);
			}
		}
	}

	if (!_uri_path.empty() && _uri_path[_uri_path.length() - 1] != '/') {
		size_t last_slash = _uri_path.find_last_of('/');
		if (last_slash != std::string::npos) {
			std::string possible_name = _uri_path.substr(last_slash + 1);
			if (possible_name.find('.') != std::string::npos) {
				return possible_name;
			}
		}
	}

	return "";
}
/* Random Functions */
void	Request::_parseUriPath()
{
	if (_uri_path.length() >= 7 && _uri_path.substr(0, 7) == "http://") 
	{
		size_t path_start = _uri_path.find('/', 7);
		if (path_start != std::string::npos) _uri_path = _uri_path.substr(path_start);
		else _uri_path = "/";
	}
	else if (_uri_path.length() >= 8 && _uri_path.substr(0, 8) == "https://") 
	{
		size_t path_start = _uri_path.find('/', 8);
		if (path_start != std::string::npos) _uri_path = _uri_path.substr(path_start);
		else _uri_path = "/";
	}

	_uri_path = Utils::normalize_uri(Utils::urlDecode(_uri_path));
}
bool	Request::openBodyFile()
{
	_body_file_path = "/tmp/webserv_req_" + Utils::generateRandomId();
	_body_fd = open(_body_file_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (_body_fd < 0) {
		_state = STATE_ERROR;
		_code = 500;
		_sys_error_msg = std::strerror(errno);
		return false;
	}
	return true;
}

void	Request::setState(RequestState state) { _state = state; }
void	Request::setCode(short code) { _code = code; }
void Request::setMaxBodySize(size_t max_size) { _max_body_size = max_size; }
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
				if (_chunk_length == 0) { close(_body_fd); _chunk_state = PAYLOAD_BODY_CR; }
				else
				{
					if ((_total_body_bytes + _chunk_length) > _max_body_size) {
						std::cout << "[Security] Blocked chunk! Exceeds max_body_size.\n";
						_state = STATE_ERROR;
						_code = 413; // 413 Payload Too Large
						break; 
					}

					_chunk_state = PAYLOAD_BODY;
					_chunk_bytes_read = 0; }
			}
			else { _state = STATE_ERROR; _code = 400; }
			break ;
		}
		case PAYLOAD_BODY:
		{
			if (_body_fd < 0) { _state = STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break; }
			size_t needed = _chunk_length - _chunk_bytes_read;
			size_t available = size - i;
			size_t to_write = std::min(needed, available);
			if (to_write > 0) {
				ssize_t bytes_written = write(_body_fd, data + i, to_write);
				if (bytes_written <= 0) { _state = STATE_ERROR; _code = 500; _sys_error_msg = std::strerror(errno); break; }
				_chunk_bytes_read += bytes_written;
				_total_body_bytes += bytes_written;
				i += bytes_written - 1;
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

/* RFC validation */

/* * RFC 3986, Section 2.3: Unreserved Characters
 * unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
 */
bool Request::_is_unreserved(unsigned char c)
{
	return (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~');
}

/* * RFC 3986, Section 2.2: Reserved Characters (Sub-delims)
 * sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
 */
bool Request::_is_sub_delim(unsigned char c)
{
	std::string sub_delims = "!$&'()*+,;=";
	return (sub_delims.find(c) != std::string::npos);
}

/* * RFC 3986, Section 3.3: Path
 * pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
 */
bool Request::_is_pchar(unsigned char c)
{
	return (_is_unreserved(c) || _is_sub_delim(c) || c == ':' || c == '@' || c == '%');
}

/* * RFC 3986, Section 3.4: Query
 * query = *( pchar / "/" / "?" )
 * Note: The query component is indicated by the first question 
 * mark ("?") character and terminated by a number sign ("#") character.
 */
bool Request::_is_query_char(unsigned char c)
{
	return (_is_pchar(c) || c == '/' || c == '?');
}

/* * RFC 7230, Section 3.2.6: Field Value Components (Token)
 * token = 1*tchar
 * tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." / 
 * "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
 */
bool Request::_is_token(unsigned char c)
{
	if (isalnum(c)) return true;
	std::string symbols = "!#$%&'*+-.^_`|~";
	return (symbols.find(c) != std::string::npos);
}

/* * RFC 7230, Section 3.2.6: Field Value Components
 * field-vchar = VCHAR / obs-text
 * VCHAR = %x21-7E (Visible ASCII)
 * obs-text = %x80-FF (Extended ASCII/Latins)
 * White space (SP / HTAB) is allowed between tokens.
 */
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
/* Request Reseter */
void	Request::resetState()
{
	if (_body_fd >= 0) {
		close(_body_fd);
		_body_fd = -1;
	}
	if (_state != STATE_COMPLETE  && !_body_file_path.empty()) {
		std::remove(_body_file_path.c_str());
		_body_file_path.clear();
	}
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
	_is_cgi = false;
	_has_content_length = 0;
	_max_body_size = 0;
}
/* Constructors */
Request::Request(short& code) :
	_state(STATE_METHOD),
	_code(code),
	_body_fd(-1),
	_has_content_length(false),
	_body_exist(false),
	_chunked_transfer(false),
	_keep_alive(true),
	_max_body_size(0),
	_content_length(0),
	_total_body_bytes(0),
	_chunk_length(0),
	_chunk_bytes_read(0),
	_chunk_state(LENGTH),
	_is_cgi(false)
{}
Request::~Request()
{
	close(_body_fd);
	if (_state != STATE_COMPLETE && !_body_file_path.empty())
		std::remove(_body_file_path.c_str());
	std::cout << RED << "Request Destructor called. Cleaning up resources." << RESET << std::endl;
}
Request::Request(const Request& other) : _code(other._code) { *this = other; }
Request& Request::operator=(const Request& other)
{
	if (this != &other) {
		_body_fd = other._body_fd;
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
		_has_content_length = other._has_content_length;
		_max_body_size = other._max_body_size;
	}
	return (*this);
}
