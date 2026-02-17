#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>

void Client::onReadable()
{
	char buf[BUFFER_SIZE];
	ssize_t bytes = recv(_fd, buf, sizeof(buf), 0);
	if (bytes > 0) {
		_request.feed(buf, bytes);
		if (_request.getState() >= STATE_HEADERS_DONE) markProcessing();
	}
	else {
		std::cerr << "Client disconnected or Read Error on fd=" << _fd << std::endl;
		_closed = true;
	}
}
/* The Bridge / Worker */
void Client::process()
{
	if (!hasConfig())
		_setBestConfig();

	if (_request.getState() == STATE_ERROR) {
		std::cout << "[Bridge] Request Error detected early." << std::endl;
		_response.generateError();
		markWriting();
		return;
	}
}
void Client::onWritable()
{}
/* Getters */
int Client::getFd() const { return _fd; }
bool Client::isClosed() const { return _closed; }
Request& Client::getRequest() { return _request; }
const Directives* Client::getConfig() const { return _config; }
const Server* Client::getServer() const { return _server; }
const short& Client::getStatus() const { return _status_code; }
void Client::setStatus(int status) { _status_code = status; }
ClientState Client::getState() const { return _state; }
/* Context => Server and Directives */
void Client::_setBestConfig()
{}
bool Client::hasConfig() const { return _server != NULL; }
/* State methods */
void Client::markProcessing() { _state = PROCESSING; }
void Client::markWriting() { _state = WRITING; }
void Client::scheduleClose() { _closed = true; }
/* Constructors */
Client::Client(int fd, const std::vector<Server*>& server_candidates)
	: _fd(fd), 
	_state(READING), 
	_closed(false),
	_status_code(200),
	_request(_status_code),
	_response(_status_code),
	_server_candidates(&server_candidates),
	_server(server_candidates[0]),
	_config(NULL)
{
	std::cout << "🟢 New Client fd=" << _fd << std::endl;
}
Client::~Client()
{
    std::cout << "🔴 Closing Client fd=" << _fd << std::endl;
    if (_fd != -1)
        close(_fd);
}