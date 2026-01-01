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
				_state = STATE_COMPLETE;
				return ;
			}
			case STATE_FIELD_START:
			{
				if (ch == '\r') {//we finish header parsing
					_state = STATE_FIELD_END;
				}
				else (Request::_is_token(ch)) {
					_state = STATE_FIELD;
				}
				else throw RequestException(400);
				break ;
			}

			case STATE_COMPLETE:
				return ;
		}
		_tmp_buffer += ch;
	}
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
	_content_length(0)
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
