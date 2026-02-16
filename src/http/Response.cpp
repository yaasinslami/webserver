#include "Response.hpp"

void	Response::generateError()
{
	
}

/* Constructors */
Response::Response(short& code) : _code(code) {}
Response::Response(const Response& other) : _code(other._code) { *this = other; }
Response& Response::operator=(const Response& other)
{
	if (this != &other) {
		_code = other._code;
	}
	return *this;
}
Response::~Response() {}