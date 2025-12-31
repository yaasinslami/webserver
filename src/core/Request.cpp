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
				++_m_index;
				_state = STATE_METHOD;
				break ;
			}
			case STATE_METHOD:
			{
				if (ch != _method_str[_method][++_m_index]) {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(501);
				}
				if ((size_t)_m_index == _method_str[_state].length())
					_state = STATE_SPACE_BEFORE_URI;
				break ;
			}
			case STATE_SPACE_BEFORE_URI:
			{
				if (ch != ' ') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_URI_SLASH;
				continue ;
			}
			case STATE_URI_SLASH:
			{
				if (ch != '/') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_URI_PATH;
				break ;
			}
			case STATE_URI_PATH:
			{
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
				break; 
			}
			case STATE_PROTOCOL_HT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HTT;
				break; 
			}
			case STATE_PROTOCOL_HTT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HTTP;
				break; 
			}
			case STATE_PROTOCOL_HTTP:
			{
				if (ch != 'P') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_CR;
				break; 
			}
		}

		_tmp_buffer += ch;
	}
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
