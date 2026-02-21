#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <algorithm>

void Client::onReadable()
{
	char buf[BUFFER_SIZE];

		ssize_t bytes = recv(_fd, buf, sizeof(buf), 0);
		if (bytes > 0) {
			_request.feed(buf, bytes);
		}
		else if (bytes == 0) {
			std::cout << "Client closed fd=" << _fd << std::endl;
			_closed = true;
			setState(CLOSING);
		}
}


static string get_cgi_executable(string path, Client& client)
{
	const Directives* config = client.getConfig();
	const map<string, string>& handlers = config->getCgiHandlers();

	if (handlers.empty())
		return "";

	size_t dot_pos = path.rfind('.');
	if (dot_pos == string::npos)
		return "";

	string extension = path.substr(dot_pos);

	map<string, string>::const_iterator it = handlers.find(extension);

	if (it != handlers.end())
	{
		client.getRequest().setIsCgi(true);
		return it->second;
	}

	return "";
}

void Client::process(EpollManager &poller)
{
	if (!_server) { 
        std::cout << RED << "CRITICAL: _server is NULL in process()" << RESET << std::endl; 
        return; 
    }

	cout << GREEN << "process path : " << _request.getPath() << RESET << endl;

	if (_request.getState() == STATE_ERROR) {
		std::cout << "[Bridge] Fatal parser error detected early (Code: " << _request.getCode() << ")." << std::endl;
		// _config = &_server->getDirectives();
		_response.setConfig(_config);
		// _response.generateError();
		markWriting();
		poller.set_events(_fd, EPOLLOUT);
		return;
	}

	if (!hasConfig())
	{
		_setBestConfig();
		_response.setConfig(_config);
	}

	if (!_validateRequest())
	{
		std::cout << "[Client] Request rejected. Code: " << _status_code << std::endl;
		_request.setState(STATE_ERROR);
		// _response.generateError();
		markWriting();
		return;
	}

	if (_request.getState() == STATE_COMPLETE) {
		// CGI
		std::string executable_path = get_cgi_executable(_request.getPath(), *this);
		if (_request.getIsCgi())
		{
			cout << BLUE << "CGI Request" << RESET << std::endl;
			std::string script_fs_path = CgiHandler::prepare(this, poller, _fd);
			if (script_fs_path.empty()) {
				// _response.generateError();
				markWriting();
				poller.set_events(_fd, EPOLLOUT);
				return;
			}
			CgiHandler::execute(this, script_fs_path, executable_path, poller);
		}
		else
		{
			// Normal Response 
			cout << BLUE << "Normal Request" << RESET << std::endl;
			_response.generate(_request);
			markWriting();
			poller.set_events(_fd, EPOLLOUT);
		}
	}
}

bool	Client::_validateRequest()
{
	/* allowed Methods */
	bool	method_allowd = false;
	const std::vector<std::string> &allowed = _config->getAllowedMethods();
	if (std::find(allowed.begin(), allowed.end(), _request.getMethod()) == allowed.end()) {
		_status_code = 405;
		return false;
	}
	/* Max Body Size */
	if (_request.hasBody()) {
		if (_request.getContentLength() > _config->getMaxBodySize() || _request.getBodySize() > _config->getMaxBodySize()) {
			_status_code = 413; 
			return false;
		}
	}
	return true;
}

void	Client::_setBestConfig()
{
	/* Server */
	std::string host = _request.getHeader("host");
	size_t colon_pos = host.find(':');
	if (colon_pos != std::string::npos) host = host.substr(0, colon_pos);

	for (size_t i = 0; i < _server_candidates->size(); ++i) {
		Server* srv = (*_server_candidates)[i];
		if (srv->hasServerName(host)) { _server = srv; break; }
	}

	/* Location */
	const std::vector<Location>& locations = _server->getLocation();
	const Location *best_match = NULL;
	std::string path = _request.getPath();
	size_t best_len = 0;
	for (size_t i = 0; i < locations.size(); ++i) {
		const std::string &loc_path = locations[i].getDirectives().getPath();
		if (path.compare(0, loc_path.length(), loc_path) == 0) {
			if (path.length() == loc_path.length() || path[loc_path.length()] == '/') {
				if (loc_path.length() > best_len) {
					best_match = &locations[i];
					best_len = loc_path.length();
				}
			}
		}
	}
	_config = (best_match) ? &best_match->getDirectives() : &_server->getDirectives();
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
		std::cout << "reset the state\n";
		resetForNextRequest();
	}
}

void Client::parseCgiResponse()
{
	std::string raw = _cgi_response_buf;
	size_t split_pos = raw.find("\r\n\r\n");
	size_t sep_len = 4;

	if (split_pos == std::string::npos) {
		split_pos = raw.find("\n\n");
		sep_len = 2;
	}

	if (split_pos == std::string::npos) {
		std::cerr << "[Error] CGI sent no header separator." << std::endl;
		setStatus(502);
		return;
	}

	std::string headers_part = raw.substr(0, split_pos);
	std::string body_part = raw.substr(split_pos + sep_len);

	std::string status_line = "HTTP/1.1 200 OK";
	std::string final_headers = "";
	std::stringstream ss(headers_part);
	std::string line;

	while (std::getline(ss, line)) {
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);
		if (line.empty()) continue;

		if (line.compare(0, 7, "Status:") == 0) {
			if (line.size() > 8) status_line = "HTTP/1.1 " + line.substr(8);
		} else if (line.find(':') == std::string::npos) {
			setStatus(502);
			return;
		} else
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
bool	Client::hasConfig() const { return _config != NULL; }
void	Client::resetForNextRequest() {
    _request.resetState();
    // _response.clear();
    _config = NULL;
    _status_code = 200;
    _state = READING;
}
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