#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>

void	Request::feed(const char* chunk, size_t size)
{
	_buffer.append(chunk, size);
	
	size_t	header_end = _buffer.find("\r\n\r\n");

	if (_state == STATE_READING_HEADERS)
	{
		if (header_end != std::string::npos)
		{
			std::string	header_str = _buffer.substr(0, header_end);
			try {
				parse_header(header_str);
				_state = STATE_READING_BODY;
			}
			catch(std::exception& e) {
				std::cerr << "Request Error: " << e.what() << std::endl;
				_state = STATE_ERROR;
			}
		}
	}
	else if (_state == STATE_READING_BODY)
	{
		
	}
}

void	Request::parse_header(const std::string& header)
{
	std::stringstream ss(header);
	std::string	line;

	// before start parsing headers, parse the request line first
	if (!std::getline(ss, line)) throw RequestException(400);

	if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);

	_parse_request_line(line);

	// loop over header lines
	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r') {
			line.erase(line.size() - 1);
		}

		if (line.empty()) continue ;

		size_t	delim_pos = line.find(':');
		if (delim_pos == std::string::npos)
			throw	RequestException(400);

		// trim here
		
		_headers[line.substr(0, delim_pos)] = line.substr(delim_pos + 1);
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

Request::Request() : _state(STATE_READING_HEADERS), _content_length(0) {}

Request::~Request() {}

std::string Request::getMethod() const { return _method; }
std::string Request::getPath() const { return _path; }
std::string Request::getVersion() const { return _version; }

Request::RequestException::RequestException(int code) : _status_code(code) {}
int Request::RequestException::get_status_code() const { return _status_code; }
const char* Request::RequestException::what() const throw() { return "Request Parsing Error"; }

/* Request::parse()
void	Request::parse(std::string content)
{
	std::stringstream	ss(content);

	// Request line
	std::string line;
	if (!std::getline(ss, line)) throw RequestException(400); // Empty request

	if (!line.empty() && line[line.length() - 1] == '\r')
		line.erase(line.length() - 1);
	else
		throw RequestException(401);

	std::stringstream	line_stream(line);
	std::string		token;
	std::vector<std::string> tokens;

	while (std::getline(line_stream, token, ' ')) {
		if (!token.empty()) tokens.push_back(token);
	}
	if (tokens.size() != 3) throw RequestException(402);

	_method = tokens[0];
	_path = tokens[1];
	_version = tokens[2];
	if (_version != "HTTP/1.1") throw RequestException(505);

	// loop over request header
	while (std::getline(ss, line))
	{
		
		std::cout << line << std::endl;
	}
	// if (line != "\r") throw RequestException(506);

	// request Header
	// while (std::getline(ss, line))
	// {
	// 	// parse the Host
	// 	std::string	host;

	// 	if (line.size())
	// }
}*/