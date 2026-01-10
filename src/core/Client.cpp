#include "Client.hpp"
#include <iostream>

void	Client::show() const
{
	std::cout << "------ Client (" << _fd << ") ------\n";

	_request.show();
}

std::string& Client::getRequestBuffer()
{
	return _request_buffer;
}

Client::Client() : _fd(-1) {}

Client::Client(int fd) : _fd(fd) {}

Client::Client(const Client& other) { *this = other; }

Client& Client::operator=(const Client& other)
{
	if (this != &other)
	{
		_fd = other._fd;
		_request = other._request;
	}
	return *this;
}

Client::~Client() {}

bool	Client::isRequestReady() const
{
	return _request.requestCompleted();
}

Request& Client::getRequest() { return _request; }