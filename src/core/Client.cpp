#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <sstream>

void Client::onReadable()
{
	char buf[BUFFER_SIZE];

		ssize_t bytes = recv(_fd, buf, sizeof(buf), 0);
		if (bytes > 0) {
			std::cout << "DEBUG: Try to Feed: [" << bytes << "] | Request State: " << _request.getState() << std::endl;
			_request.feed(buf, bytes);
			std::cout << "DEBUG: Bytes read: " << bytes << " | Request State: " << _request.getState() << std::endl;


			if (_request.getState() == STATE_COMPLETE)
			{
				std::cout << GREEN << "Request Completed" << RESET << std::endl;
				markProcessing();
			}
		}
		else if (bytes == 0) {
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
		_state = CLOSING;
		_closed = true;
	}
	else if (_write_buf.empty())
	{
		std::cout <<  "reset the state\n";
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

/* Getters */
ClientState Client::getState() const { return _state; }
int Client::getFd() const { return _fd; }
bool Client::isClosed() const { return _closed; }
Request& Client::getRequest() { return _request; }
const Directives* Client::getConfig() const { return _config; }
const Server* Client::getServer() const { return _server; }
const short& Client::getStatus() const { return _status_code; }
void Client::setStatus(int status) { _status_code = status; }
void Client::setState( ClientState state ) { _state = state; }
bool	Client::isRequestReady() { return _request.requestCompleted(); }
time_t	Client::getLastActivity( void ) const { return _last_activity_time; }
/* CGI */
void Client::setCgiPid(pid_t pid) { _cgi_pid = pid; };
pid_t Client::getCgiPid() const { return _cgi_pid; };
void Client::setCgiOutPipe(int fd) { _cgi_pipe_read_fd = fd; }
int Client::getCgiOutPipe() const { return _cgi_pipe_read_fd; }
string&	Client::get_cgi_response_buf() { return _cgi_response_buf; }
size_t&	Client::get_cgi_bytes_written() { return _cgi_bytes_written; }
/* timeout */
void	Client::updateTime() { _last_activity_time = time(NULL); }
/* State methods */
void Client::markProcessing() { _state = PROCESSING; }
void Client::markWriting() { _state = WRITING; }
void Client::scheduleClose() { _closed = true; }
/* Constructors */
Client::Client(int fd, const std::vector<Server*>& server_candidates) :
	_fd(fd), 
	_state(READING), 
	_closed(false),
	_status_code(200),
	_request(_status_code),
	_response(_status_code),
	_server_candidates(&server_candidates),
	_server(server_candidates[0]), 
	_config(NULL), 
	_cgi_pid(-1),
	_cgi_pipe_read_fd(-1),
	_cgi_bytes_written(0),
	_last_activity_time(time(NULL))
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

// i will remove this
const std::string& Client::get_write_buf() const
{
	return _write_buf;
}

void Client::set_write_buf(const std::string &s)
{
	_write_buf = s;
}