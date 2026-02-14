#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>

Client::Client(int fd, Server* owner_server)
    : _fd(fd), 
      _state(READING), 
      _closed(false),
      _server(owner_server), 
      _config(NULL), 
      _status_code(200)
{
    std::cout << "🟢 New Client fd=" << _fd << std::endl;
}

Client::~Client()
{
    std::cout << "🔴 Closing Client fd=" << _fd << std::endl;
    if (_fd != -1)
        close(_fd);
}

void Client::onReadable()
{
	char buf[BUFFER_SIZE];

	ssize_t bytes = recv(_fd, buf, sizeof(buf), 0);
	if (bytes > 0) {
		_request.feed(buf, bytes);
		// check here if we finish headers -> to see if something bad happen
	}
	else {
		cerr << "Client disconnected or Read Error on fd=" << _fd << std::endl;
		_closed = true;
	}
}

void Client::onWritable()
{}

int Client::getFd() const { return _fd; }
bool Client::isClosed() const { return _closed; }

Request& Client::getRequest() { return _request; }
const Directives* Client::getConfig() const { return _config; }
const Server* Client::getServer() const { return _server; }

int& Client::getStatus() { return _status_code; }
void Client::setStatus(int status) { _status_code = status; }

void Client::setContext(Server *s, const Location *l)
{
    _server = s;
    if (l != NULL)
        _config = &l->getDirectives();
    else if (s != NULL)
        _config = &s->getDirectives();
}

bool Client::hasContext() const { return _server != NULL; }

ClientState Client::getState() const { return _state; }

void Client::markProcessing() { _state = PROCESSING; }
void Client::markWriting() { _state = WRITING; }
void Client::scheduleClose() { _closed = true; }
