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
    if (_server)
        _config = &_server->getDirectives();
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
        if (bytes > 0)
        {
            _read_buf.append(buf, bytes);
            ssize_t consumed = _request.feed(buf, bytes);
			if (consumed == -1) {
                cerr << "Bad Request" << endl;
                setStatus(400);
                markProcessing();
                return ;
            }

            cout << "consumed : " << consumed << endl; 

            _request.show();
        }
        else if (bytes == 0)
        {
            std::cout << "Client closed fd=" << _fd << std::endl;
            _closed = true;
        }
}

void Client::onWritable()
{
    ssize_t sent = send(_fd, _write_buf.c_str(), _write_buf.size(), 0);

    if (sent > 0)
        _write_buf.erase(0, sent);
    else
        return;
    
    if (_write_buf.empty() && !_request.shouldKeepAlive())
    {
         _closed = true;
    }
    else if (_write_buf.empty())
    {
        _state = READING;
        _request.resetState();
    }
}


int Client::getFd() const { return _fd; }
bool Client::isClosed() const { return _closed; }

Request& Client::getRequest() { return _request; }
const Directives* Client::getConfig() const { return _config; }
Server* Client::getServer() const { return _server; }

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

bool Client::isRequestReady() 
{
    return _request.requestCompleted(); 
}

ClientState Client::getState() const { return _state; }

void Client::markProcessing() { _state = PROCESSING; }
void Client::markWriting() { _state = WRITING; }
void Client::setWriteBuf(const std::string &s) { _write_buf = s; }
void Client::scheduleClose() { _closed = true; }

const std::string& Client::get_read_buf() const
{
	return _read_buf;
}

const std::string& Client::get_write_buf() const
{
	return _write_buf;
}

void Client::set_write_buf(const std::string &s)
{
	_write_buf = s;
}
