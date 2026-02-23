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

static string	get_cgi_executable(string path, Client& client)
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

void	Client::process(EpollManager &poller)
{
	if (!hasConfig()) {
		if (_request.getState() != STATE_ERROR) _setBestConfig();
		else _config = &_server->getDirectives();
		_response.setConfig(_config);	
	}

	if (_request.getState() != STATE_ERROR && !_validateRequest()) {
		std::cout << "[Client] Request rejected. Code: " << _status_code << std::endl;
		_request.setState(STATE_ERROR);
		_response.generateError();
		markWriting();
		return;
	}

	if (_request.getState() == STATE_COMPLETE) {
		if (!_request.getIsCgi()) {
			cout << BLUE << "Normal Request" << RESET << std::endl;
			_response.generate(_request);
			if (_status_code >= 400) {
				std::cout << "[Client] Error response generated. Code: " << _status_code << std::endl;
				_request.setState(STATE_ERROR);
			}
		}
		else {
			std::string executable_path = get_cgi_executable(_request.getPath(), *this);
			std::string script_fs_path = CgiHandler::prepare(this, poller, _fd);
			if (script_fs_path.empty())
			{
				markWriting();
				poller.set_events(_fd, EPOLLOUT);
				return;
			}
			CgiHandler::execute(this, script_fs_path, executable_path, poller);
		}
	}

	if (_request.getState() == STATE_ERROR) {
		std::cout << "[Client] Generating Error Page for Code: " << _status_code << std::endl;
		_response.generateError();
		markWriting();
		poller.set_events(_fd, EPOLLOUT);
		return;
	}

	if (_request.getState() == STATE_COMPLETE) {
		markWriting();
		poller.set_events(_fd, EPOLLOUT);
	}
}

bool Client::_validateRequest()
{
	if (!_request.headersValidated())
	{
		/* Allowed Methods */
		const std::vector<std::string> &allowed = _config->getAllowedMethods();
		if (std::find(allowed.begin(), allowed.end(), _request.getMethod()) == allowed.end())
		{
			_status_code = 405; // Method Not Allowed
			return false;
		}
		/* Content-Length */
		if (_request.hasBody() && _request.getContentLength() > _config->getMaxBodySize())
		{
			_status_code = 413; // Payload Too Large
			return false;
		}

		/* Path Translation */
		_response.translatePath(_request.getPath());
		std::cout << "[Response] Target resource: " << _response.getPhysicalPath() << std::endl;
		_request.markHeadersValidated();
	}

	if (_request.hasBody() && _request.getBodySize() > _config->getMaxBodySize()) {
		_status_code = 413;
		return false;
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

void	Client::_serveLargeFile()
{
	if (!_file_stream.is_open())
	{
		_file_stream.open(_response.getPhysicalPath().c_str(), std::ios::binary);
		if (_request.getIsCgi())
		{
			_file_stream.seekg(_cgi_header_size, std::ios::beg);
			std::cout << RED << "Seeked to: " << _file_stream.tellg() << RESET << std::endl;
		}
	}

	char buf[BUFFER_SIZE];
	_file_stream.read(buf, BUFFER_SIZE);
	ssize_t bytes_read = _file_stream.gcount();

	if (bytes_read > 0) {
		ssize_t sent = send(_fd, buf, bytes_read, 0);
		if (sent < bytes_read)
		{
			long unsent = bytes_read - (sent > 0 ? sent : 0);
			_file_stream.seekg((long)_file_stream.tellg() - unsent);
		}
	}

	if (_file_stream.eof())
	{
		_file_stream.close();
		_file_stream.clear();

		if (!_response.getPhysicalPath().empty())
			unlink(_response.getPhysicalPath().c_str());
		_response.clearFilePath();
	}
}

void	Client::onWritable()
{
	if (!_response.getHeaderBuffer().empty())
	{
		cout << RED << "[DEBUG] : send Headers" << RESET << endl;

		std::string& h_buf = _response.getHeaderBuffer();
		ssize_t sent = send(_fd, h_buf.c_str(), h_buf.size(), 0);
		if (sent > 0) h_buf.erase(0, sent);
		
		if (!h_buf.empty()) return;
	}

	if (!_response.getBuffer().empty())
	{
		cout << RED << "[DEBUG] : send _response._body_content in case of error" << RESET << endl;

		std::string& b_buf = _response.getBuffer();
		ssize_t sent = send(_fd, b_buf.c_str(), b_buf.size(), 0);
		if (sent > 0) b_buf.erase(0, sent);

		if (!b_buf.empty()) return;
	}

	if (!_response.getPhysicalPath().empty())
	{
		cout << RED << "[DEBUG] : send body from File" << RESET << endl;
		_serveLargeFile();
	}

	if (_response.getHeaderBuffer().empty() && 
		_response.getBuffer().empty() && 
		_response.getPhysicalPath().empty())
	{
		cout << RED << "Sending DONE" << RESET << endl;
		if (!_request.shouldKeepAlive() || _status_code >= 400)
		{
			_state = CLOSING;
			_closed = true;
		}
		else resetForNextRequest();
	}
}

void Client::parseCgiResponse()
{
	std::string& raw = _cgi_header_buf;
	size_t split_pos = raw.find("\r\n\r\n");
	size_t sep_len = 4;

	if (split_pos == std::string::npos) {
		split_pos = raw.find("\n\n");
		sep_len = 2;
	}

	if (split_pos == std::string::npos) {
		setStatus(502);
		return;
	}

	std::string headers_part = raw.substr(0, split_pos);
	_cgi_header_size = split_pos + sep_len;

	cout << BLUE << headers_part << RESET << endl;

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
		else if (line.find(':') == std::string::npos)
		{
			setStatus(502);
			return;
		}
		else
			final_headers += line + "\r\n";
	}

	// 2. Calculate Body Size from the file we just wrote
	long total_file_size = Utils::getFileSize(_cgi_temp_path);
	long body_size = total_file_size - _cgi_header_size;

	// 3. Set the Response Object
	std::stringstream h_ss;
	h_ss << status_line << "\r\n";
	h_ss << final_headers;
	h_ss << "Content-Length: " << body_size << "\r\n";
	h_ss << "\r\n";

	_response.getHeaderBuffer() = h_ss.str();
	_response.setPath(_cgi_temp_path);
}

void Client::openCgiTempFile()
{
	char filename[] = "/tmp/webserv_cgi_XXXXXX";

	int fd = mkstemp(filename);
	if (fd == -1)
	{
		_status_code = 500;
		return;
	}

	_cgi_temp_path = filename;

	_cgi_out_file.open(_cgi_temp_path.c_str(), std::ios::binary);
	close(fd);
}

void Client::writeToCgiTempFile(const char* buf, ssize_t size)
{
	if (_cgi_out_file.is_open())
		_cgi_out_file.write(buf, size);
}

void Client::closeCgiOutFile()
{
	if (_cgi_out_file.is_open())
	{
		_cgi_out_file.close();
		_cgi_out_file.clear();
	}
}

bool Client::isCgiFileOpen() const 
{
    return const_cast<std::ofstream&>(_cgi_out_file).is_open();
}

std::string& Client::getCgiHeaderBuf()
{
    return _cgi_header_buf;
}

/* Getters */
ClientState Client::getState() const { return _state; }
int Client::getFd() const { return _fd; }
bool Client::isClosed() const { return _closed; }
Request& Client::getRequest() { return _request; }
Response& Client::getResponse() { return _response; }
const Directives* Client::getConfig() const { return _config; }
const Server* Client::getServer() const { return _server; }
const short& Client::getStatus() const { return _status_code; }
void Client::setStatus(int status) { _status_code = status; }
void Client::setState( ClientState state ) { _state = state; }
bool	Client::isRequestReady() { return _request.requestCompleted(); }
time_t	Client::getLastActivity( void ) const { return _last_activity_time; }
bool	Client::hasConfig() const { return _config != NULL; }
void	Client::resetForNextRequest()
{
	if (_file_stream.is_open())
	{
		_file_stream.close();
		_file_stream.clear();
	}

	closeCgiOutFile();

	_cgi_temp_path.clear();
	_request.resetState();
	_response.clear();
	_cgi_pid = -1;
    _cgi_header_buf.clear();
    _cgi_header_size = 0;
	_config = NULL;
    _status_code = 200;
    _state = READING;
}
/* CGI */
void Client::setCgiPid(pid_t pid) { _cgi_pid = pid; };
pid_t Client::getCgiPid() const { return _cgi_pid; };
void Client::setCgiOutPipe(int fd) { _cgi_pipe_read_fd = fd; }
int Client::getCgiOutPipe() const { return _cgi_pipe_read_fd; }
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
	_last_activity_time(time(NULL)),
	_file_stream(),
	_cgi_out_file(),
	_cgi_header_size(0)
{
	std::cout << "🟢 New Client fd=" << _fd << std::endl;
}
Client::~Client()
{
	std::cout << "🔴 Closing Client fd=" << _fd << std::endl;
	if (_file_stream.is_open())
		_file_stream.close();

	if (_fd != -1)
		close(_fd);
}
