#include "Client.hpp"
#include <iostream>

bool	Client::shouldClose() const
{
	return _should_close;
}

void	Client::setClose(bool closeFlag)
{
	_should_close = closeFlag;
}

bool	Client::shouldKeepAlive() const
{
	return _request.shouldKeepAlive();
}

bool Client::hasDataToSend() const
{
	return !_response_buffer.empty();
}

void Client::updateResponseBuffer(size_t n)
{
	if (n >= _response_buffer.length())
		_response_buffer.clear();
	else
		_response_buffer = _response_buffer.substr(n);
}

int	Client::getFd()
{
	return _fd;
}

const Location* Client::getLocation() const
{
	return _location;
}

const Server* Client::getServer() const
{
	return _server;
}

void Client::setContext(Server *s, const Location *l)
{
	_server = s;
	_location = l;
}

void	Client::show() const
{
	std::cout << "------ Client (" << _fd << ") ------\n";

	_request.show();
}

std::string& Client::getRequestBuffer()
{
	return _request_buffer;
}

const char* Client::getResponseBuffer() const
{
	return _response_buffer.c_str();
}

size_t	Client::getResponseSize() const
{
	return _response_buffer.length();
}

Client::Client() : _fd(-1), _server_port(0), _should_close(false),_server(NULL), _location(NULL) {}

Client::Client(int fd, unsigned short server_port) : _fd(fd), _server_port(server_port), _should_close(false), _server(NULL), _location(NULL) {}

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
	return _request.requestCompleted() || _request.badRequest();
}

Request& Client::getRequest() { return _request; }

const Request& Client::getRequest() const { return _request; }

unsigned short	Client::getServerPort() const
{
	return _server_port;
}

void	Client::setResponseBuffer(const std::string& response_content)
{
	_response_buffer += response_content;
}