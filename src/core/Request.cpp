#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>

void	Request::feed(const char* chunk, size_t size)
{
	std::cout << "feed\n";
	_buffer.append(chunk, size);
	
	size_t	header_end = _buffer.find("\r\n\r\n");

	if (_state == STATE_READING_HEADERS)
	{
		if (header_end != std::string::npos)
		{
			std::string	header_str = _buffer.substr(0, header_end);
			try {
				_parse_header(header_str);
				_state = STATE_READING_BODY;
				_check_body_complete(header_end);
				_state = STATE_COMPLETE;
			}
			catch(std::exception& e) {
				std::cerr << "Request Error: " << e.what() << std::endl;
				_state = STATE_ERROR;
			}
		}
	}
	else if (_state == STATE_READING_BODY) {
		_check_body_complete(header_end);
	}
	_content_length = 1;
}

void	Request::_parse_header(const std::string& header)
{
	std::stringstream ss(header);
	std::string	line;

	// before start parsing headers, parse the request line first
	if (!std::getline(ss, line)) throw RequestException(400);

	if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);

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
}

void	Request::_parse_request_line(const std::string& line)
{
	std::stringstream	ss(line);

	if (!(ss >> _method >> _path >> _version))
		throw RequestException(400);

	std::string	trash;
	if (ss >> trash)
		throw RequestException(400);
}

void	Request::_check_body_complete(size_t header_end)
{
	(void)header_end;
}

bool	Request::requestCompleted() const
{
	return _state == STATE_COMPLETE;
}

Request::Request() : _state(STATE_READING_HEADERS), _content_length(0) {}

Request::~Request() {}

std::string Request::getMethod() const { return _method; }
std::string Request::getPath() const { return _path; }
std::string Request::getVersion() const { return _version; }

Request::RequestException::RequestException(int code) : _status_code(code) {}
int Request::RequestException::get_status_code() const { return _status_code; }
const char* Request::RequestException::what() const throw() { return "Request Parsing Error"; }
