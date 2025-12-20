#include "Request.hpp"
#include <sstream>

Request::Request(std::string raw_content)
{
	parse(raw_content);
}

Request::~Request() {}

void	Request::parse(std::string raw)
{
	std::stringstream	ss(raw);
	// ss >> this->_method;
	// ss >> this->_path;
	// ss >> this->_version;

	
}

std::string Request::getMethod() const { return _method; }
std::string Request::getPath() const { return _path; }
std::string Request::getVersion() const { return _version; }