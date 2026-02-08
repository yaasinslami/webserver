#include "Client.hpp"
#include <iostream>

bool	Client::shouldClose() const { return _should_close; }

void	Client::setClose(bool closeFlag) { _should_close = closeFlag; }

bool	Client::shouldKeepAlive() const { return _request.shouldKeepAlive(); }

bool	Client::hasContext() const { return _server != NULL; }

int	Client::getFd() { return _fd; }

const Server* Client::getServer() const { return _server; }

const Directives* Client::getConfig() const { return _config; }

void Client::setContext(Server *s, const Location *l)
{
	_server = s;
	if (l != NULL)
		_config = &l->getDirectives();
	else
	_config = &s->getDirectives();
}

int&	Client::getStatus() { return _status_code; }

void	Client::setStatus(int status_code) { _status_code = status_code; }

void	Client::show() const
{
	std::cout << "------ Client (" << _fd << ") ------\n";
	_request.show();
}

Client::Client() : _fd(-1), _server_port(0), _should_close(false), _status_code(200), _server(NULL), _config(NULL)
{}

Client::Client(int fd, unsigned short server_port) : _fd(fd), _server_port(server_port), _should_close(false), _status_code(200), _server(NULL), _config(NULL) {}

Client::Client(const Client& other) { *this = other; }

Client& Client::operator=(const Client& other)
{
	if (this != &other) {
		_fd = other._fd;
		_server_port = other._server_port;
		_request = other._request;
	}
	return *this;
}

Client::~Client() {}

bool	Client::isRequestReady() { return _request.requestCompleted(); }

Request& Client::getRequest() { return _request; }

const Request& Client::getRequest() const { return _request; }

unsigned short	Client::getServerPort() const { return _server_port; }
