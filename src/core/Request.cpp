#include "Request.hpp"
#include <sstream>
#include <vector>

Request::Request(std::string raw_content)
{
	parse(raw_content);
}

Request::~Request() {}

/*
	GET /api/users/123 HTTP/1.1
	Host: api.example.com
	User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
	Accept: application/json
	Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
	Connection: keep-alive
*/

void	Request::parse(std::string raw)
{
	std::stringstream	ss(raw);
	// ss >> this->_method;
	// ss >> this->_path;
	// ss >> this->_version;

	std::string line;

	if (!std::getline(ss, line)) throw RequestException(400); // Empty request

	if (!line.empty() && line[line.length() - 1] == '\r') line.erase(line.length() - 1);

	std::stringstream	line_stream(line);
	std::string		token;
	std::vector<std::string> tokens;

	while (std::getline(line_stream, token, ' ')) {
		if (!token.empty()) tokens.push_back(token);
	}

	if (tokens.size() != 3) throw RequestException(400);

	_method = tokens[0];
	_path = token[1];
	_version = token[2];

	if (_version != "HTTP/1.1") throw RequestException(505);

	while (std::getline(line_stream, line))
	{
		
	}
}

// exception: Bad request

std::string Request::getMethod() const { return _method; }
std::string Request::getPath() const { return _path; }
std::string Request::getVersion() const { return _version; }

// const char* Request::RequestException::what() throw()
// {
// 	return "";
// }