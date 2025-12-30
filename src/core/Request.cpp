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
				if (ch == 'G')
					_method = GET;
				else if (ch == 'P')
					_method = POST;
				else if (ch == 'D')
					_method = DELETE;
				else
				{
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
				_state = STATE_URI;
				continue ;
			}
			case STATE_URI:
			{
				
			}
		}
	}
}

// GET /blabla HTTP/1.1

void	Request::_parse_header(const std::string& header)
{
	std::stringstream ss(header);
	std::string	line;

	// request line first
	if (!std::getline(ss, line)) throw RequestException(400);

	if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
	else throw RequestException(400);

	_parse_request_line(line);

	// loop over header
	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r') {
			line.erase(line.size() - 1);
		}

		if (line.empty()) continue ;

		size_t	delim_pos = line.find(':');
		if (delim_pos == std::string::npos)
			throw	RequestException(400);

		std::string	key = line.substr(0, delim_pos);
		std::string	value = line.substr(delim_pos + 1);

		if (value[0] == ' ')
			value = value.substr(1);

		_headers[key] = value;
	}

	if (_headers.find("Host") == _headers.end()) throw RequestException(400);

	// here we should change the implimentation from line by line to byte by byte

	// nginx clone -> so we can clone using FSM implimentation
}

void	Request::_parse_request_line(const std::string& line)
{
	size_t	first_space = line.find(' ');
	if (first_space == std::string::npos) throw RequestException(400);

	_method = line.substr(0, first_space);
	if (_method.empty()) throw RequestException(400);

	size_t second_space = line.find(' ', first_space + 1);
	if (second_space == std::string::npos) throw RequestException(400);

	_path = line.substr(first_space + 1, second_space - first_space - 1);
	if (_path.empty()) throw RequestException(400);

	_version = line.substr(second_space + 1);
	if (_version.empty() || _version.find(' ') != std::string::npos) throw RequestException(400);
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

std::string Request::getMethod() const { return _method; }
std::string Request::getPath() const { return _path; }
std::string Request::getVersion() const { return _version; }

Request::RequestException::RequestException(int code) : _status_code(code) {}
int Request::RequestException::get_status_code() const { return _status_code; }
const char* Request::RequestException::what() const throw() { return "Bad Request"; }
