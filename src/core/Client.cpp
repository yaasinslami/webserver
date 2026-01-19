#include "Client.hpp"
#include <iostream>

// void	Client::setConfig()

void	Client::show() const
{
	std::cout << "------ Client (" << _fd << ") ------\n";

	_request.show();
}

std::string& Client::getRequestBuffer()
{
	return _request_buffer;
}

Client::Client() : _fd(-1), _server_port(0) {}

Client::Client(int fd, unsigned short server_port) : _fd(fd), _server_port(server_port) {}

Client::Client(const Client& other) { *this = other; }

Client& Client::operator=(const Client& other)
{
	if (this != &other)
	{
		_fd = other._fd;
		_server_port = other._server_port;
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

const Request& Client::getRequest() const { return _request; }

unsigned short	Client::getServerPort() const
{
	return _server_port;
}