#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <sstream>

Client::Client(int fd, Server* owner_server)
	: _fd(fd), 
	  _state(READING), 
	  _closed(false),
	  _server(owner_server), 
	  _config(NULL), 
	  _status_code(200),
	  _cgi_pid(-1),
	  _cgi_pipe_read_fd(-1),
	  _cgi_pipe_write_fd(-1),
	  _cgi_bytes_written(0)
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

			_request.show();
		}
		else if (bytes == 0)
		{
			std::cout << "Client closed fd=" << _fd << std::endl;
			_closed = true;
			setState(CLOSING);
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

void	Client::parseCgiResponse()
{
	std::string raw = _cgi_response_buf;
	std::string headers_part;
	std::string body_part;

	size_t split_pos = raw.find("\r\n\r\n");
	size_t separator_len = 4;

	if (split_pos == std::string::npos)
	{
		split_pos = raw.find("\n\n");
		separator_len = 2;
	}

	if (split_pos != std::string::npos)
	{
		headers_part = raw.substr(0, split_pos);
		body_part = raw.substr(split_pos + separator_len); 
	}
	else
		body_part = raw;

	std::string status_line = "HTTP/1.1 200 OK";
	std::string final_headers = "";
	
	std::stringstream ss(headers_part);
	std::string line;

	while (std::getline(ss, line))
	{
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);

		if (line.empty())
			continue;

		if (line.compare(0, 7, "Status:") == 0)
		{
			if (line.size() > 8)
				status_line = "HTTP/1.1 " + line.substr(8);
		}
		else if (line.compare(0, 15, "Content-Length:") == 0)
			continue;
		else 
			final_headers += line + "\r\n";
	}

	std::stringstream response;
	
	response << status_line << "\r\n";
	response << final_headers;
	response << "Content-Length: " << body_part.length() << "\r\n";
	response << "\r\n";
	response << body_part;

	_write_buf = response.str();
}

int Client::getFd() const { return _fd; }
bool Client::isClosed() const { return _closed; }

Request& Client::getRequest() { return _request; }
const Directives* Client::getConfig() const { return _config; }
Server* Client::getServer() const { return _server; }

int& Client::getStatus() { return _status_code; }
void Client::setStatus(int status) { _status_code = status; }

void Client::setState( ClientState state ) { _state = state; }

void Client::setCgiPid(pid_t pid) { _cgi_pid = pid; };
pid_t Client::getCgiPid() const { return _cgi_pid; };

void Client::setCgiOutPipe(int fd) { _cgi_pipe_read_fd = fd; }
int Client::getCgiOutPipe() const { return _cgi_pipe_read_fd; }

void Client::setCgiInPipe(int fd) { _cgi_pipe_write_fd = fd; }
int Client::getCgiInPipe() const { return _cgi_pipe_write_fd; }

string&	Client::get_cgi_response_buf() { return _cgi_response_buf; }
size_t&	Client::get_cgi_bytes_written() { return _cgi_bytes_written; }

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
