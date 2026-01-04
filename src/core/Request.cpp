#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>

/* FSM */
void	Request::feed(const char* chunk, size_t size)
{
	for (size_t i = 0; i < size; ++i)
	{
		unsigned char ch = chunk[i];
	
		switch (_state)
		{
			case STATE_START:
			{
				if (ch == 'G') _method = GET;
				else if (ch == 'P') _method = POST;
				else if (ch == 'D') _method = DELETE;
				else {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(501);
				}
				_state = STATE_METHOD;
				continue ;
			}
			case STATE_METHOD:
			{
				std::cout << "STATE_METHOD\t" << "index " << _m_index << std::endl;
				if (ch != _method_str[_method][++_m_index]) {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(501);
				}
				if ((size_t)_m_index == _method_str[_method].length() - 1)
					_state = STATE_SPACE_BEFORE_URI;
				continue ;
			}
			case STATE_SPACE_BEFORE_URI:
			{
				std::cout << "STATE_SPACE_BEFORE_URI\n";
				if (ch != ' ') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_URI_SLASH;
				continue ;
			}
			case STATE_URI_SLASH:
			{
				std::cout << "STATE_URI_SLASH\n";
				if (ch != '/') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_URI_PATH;
				break ;
			}
			case STATE_URI_PATH:
			{
				std::cout << "STATE_URI_PATH\n";
				if (ch == ' ') {
					_path = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_PROTOCOL_H;
					continue ;
				}
				break ;
			}
			case STATE_PROTOCOL_H:
			{
				if (ch != 'H') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HT;
				continue ;
			}
			case STATE_PROTOCOL_HT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HTT;
				continue ;
			}
			case STATE_PROTOCOL_HTT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HTTP;
				continue ;
			}
			case STATE_PROTOCOL_HTTP:
			{
				if (ch != 'P') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_SLASH;
				continue ;
			}
			case STATE_PROTOCOL_SLASH:
			{
				if (ch != '/') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_MAJOR_DIGIT;
				continue ;
			}
			case STATE_MAJOR_DIGIT:
			{
				if (ch != '1') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_DOT_AFTER_DIGIT;
				continue ;
			}
			case STATE_DOT_AFTER_DIGIT:
			{
				if (ch != '.') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_MINOR_DIGIT;
				continue ;
			}
			case STATE_MINOR_DIGIT:
			{
				if (ch != '1') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_FIRST_CR;
				continue ;
			}
			case STATE_FIRST_CR:
			{
				if (ch != '\r') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_FIRST_LF;
				continue ;
			}
			case STATE_FIRST_LF:
			{
				if (ch != '\n') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_FIELD_START;
				continue ;
			}
			case STATE_FIELD_START:
			{
				if (ch == '\r') {
					_state = STATE_FIELD_END;
					continue ;
				}
				else if (Request::_is_token(ch)) {
					_state = STATE_FIELD_NAME;
				}
				else throw RequestException(400);
				break ;
			}
			case STATE_FIELD_NAME:
			{
				if (ch == ':') {// we have key:value
					_state = STATE_FIELD_VALUE;
				}
				else if (!Request::_is_token(ch)) {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				break ;
			}
			case STATE_FIELD_VALUE:
			{
				if (ch == '\r') {
					_add_header();
					_state = STATE_FIELD_START;
					continue ;
				}
				break ;
			}
			case STATE_FIELD_END:
			{
				if (ch != '\n') throw RequestException(400);

				__headers();
				if (_body_exist) {
					if (_chunked_transfer)
						_state = STATE_CHUNK_START;
					else
						_state = STATE_BODY;
					continue ;
				}
				else {
					_state = STATE_COMPLETE;
					return ;
				}
			}
			case STATE_BODY:
			{
				if (_body.size() < _content_length)
					_body += ch;
				if (_body.size() == _content_length) {
					_state = STATE_COMPLETE;
					return ;
				}
				break ;
			}
			case STATE_CHUNK_START:
			{
				if (!isxdigit(ch)) {
					std::cerr << "bad chunk-length" << std::endl;
					throw RequestException(400);
				}

				this->_chunk_length = charToHex(ch);
				if (this->_chunk_length == 0)
					this->_state = STATE_CHUNK_CR;
				else
					this->_state = STATE_CHUNK_FEED;
				continue ;
			}
			case STATE_CHUNK_FEED:
			{
				if (ch == '\r') {
					this->_state = STATE_CHUNK_CR;
					continue ;
				}

				this->_buffer += ch;
				--this->_chunk_length;
				continue ;
			}
			case STATE_CHUNK_CR:
			{
				if (ch != '\r') throw RequestException(400);

				this->_state = STATE_CHUNK_LF;
				continue ;
			}
			case STATE_CHUNK_LF:
			{
				if (ch != '\n') throw RequestException(400);

				if (this->_chunk_length == 0) {
					_state = STATE_CHUNK_START;
				}
				else // ISSUE!
			}

			case STATE_COMPLETE:
				return ;
		}
		_tmp_buffer += ch;
	}
}

/*
\r\n
7/r/n
Hellowo\r\n
*/

void	Request::__headers()
{
	if (_headers.count("content-length")) {
		std::string	value = _headers["content-length"];
	
		char	*endptr;
		const char* str = value.c_str();

		errno = 0;
	
		unsigned long len = std::strtoul(str, &endptr, 10);
		if (errno == ERANGE) {
			std::cerr << "content-length too large!";
			throw RequestException(400);
		}
		
		if (endptr == str || *endptr != '\0' || value.find('-') != std::string::npos) {
			std::cerr << "bad content-length" << std::endl;
			throw RequestException(400);
		}

		_content_length = (size_t)len;
		_body_exist = true;
	}

	if (_headers.count("transfer-encoding"))
	{
		if (_headers["transfer-encoding"].find("chunked"))
		{
			_chunked_transfer = true;
		}
		_body_exist = true;
	}

	if (_headers.count("host"))
	{
		_server_name = _headers["host"];
	}

	// handle Content-Type ...
}

void	Request::_add_header()
{
	size_t	delim = _tmp_buffer.find(':');

	std::string	key = _tmp_buffer.substr(0, delim);
	std::string value = _tmp_buffer.substr(delim + 1);

	if (key.empty() || value.empty()) {
		std::cerr << "key or value can't be empty!" << std::endl;
		throw RequestException(400);
	}

	// check if the key already exist
	if (_headers.count(key)) {
		_headers[key] += ", " + value;
	}
	else
		_headers[key] = value;
	
	_tmp_buffer.clear();
}

bool Request::_is_token(unsigned char c)
{
	if (isalnum(c))
		return true;

	std::string symbols = "!#$%&'*+-.^_`|~";
	return (symbols.find(c) != std::string::npos);
}

void	Request::_check_body_complete(size_t header_end)
{
	(void)header_end;
}

bool	Request::requestCompleted() const
{
	return _state == STATE_COMPLETE;
}

void	Request::resetState()
{
	this->_state = STATE_START;
}

Request::Request() :
	_state(STATE_START),
	_m_index(0),
	_content_length(0),
	_chunked_transfer(false),
	_chunk_length(0),
	_body_exist(false)
{
	_method_str[GET] = "GET";
	_method_str[POST] = "POST";
	_method_str[DELETE] = "DELETE";
}

Request::~Request() {}

std::string Request::getPath() const { return _path; }
std::string Request::getVersion() const { return _version; }

Request::RequestException::RequestException(int code) : _status_code(code) {}
int Request::RequestException::get_status_code() const { return _status_code; }
const char* Request::RequestException::what() const throw() { return "Bad Request"; }
