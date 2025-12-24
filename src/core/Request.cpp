#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>

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
}

Request::Request(std::string content){ parse(content); }

Request::~Request() {}

std::string Request::getMethod() const { return _method; }
std::string Request::getPath() const { return _path; }
std::string Request::getVersion() const { return _version; }

Request::RequestException::RequestException(int code) : _status_code(code) {}
int Request::RequestException::get_status_code() const { return _status_code; }
const char* Request::RequestException::what() const throw() { return "Request Parsing Error"; }