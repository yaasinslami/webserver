#include "Client.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <algorithm>

/* the magic happens here */
void Client::onReadable()
{
	char buf[BUFFER_SIZE];

	ssize_t bytes = recv(_fd, buf, sizeof(buf), 0);
	if (bytes > 0)
	{
		size_t total_consumed = 0;
		while (total_consumed < (size_t)bytes && _request.getState() != STATE_COMPLETE)
		{
			std::cout << CYAN << "[DEBUG] : Feeding request with " << (bytes - total_consumed) << " bytes" << RESET << std::endl;
			size_t consumed = _request.feed(buf + total_consumed, bytes - total_consumed);
			total_consumed += consumed;

			if (_request.getState() == STATE_ERROR) {
				if (_status_code >= 500) std::cout << "[Client] System error message: " << _request.getSysErrorMsg() << std::endl;
				else std::cout << "[Client] Bad request! Code: " << _status_code << std::endl;
				_setBestConfig();
				break;
			}
			else if (_request.getState() == STATE_HEADERS_DONE) {// one time check
				_setBestConfig();

				_request.setMaxBodySize(_config->getMaxBodySize());

				if (!_request.validateHeaders()) {
					_request.setState(STATE_ERROR);
					std::cout << "[Client] Blocked request! Invalid headers.\n";
					break;
				}

				if (!_validateRequest()) {// translate path happens in validation
					std::cout << "[Client] Blocked request! Method/Size invalid.\n";
					_request.setState(STATE_ERROR);
					break;
				}

				if (_request.isBodyMethod(_request.getMethod()) && _request.hasBody()) {
					if (!_request.openBodyFile()) {
						std::cout << "[Client] Failed to open body file! Code: " << _status_code << ", Error: " << _request.getSysErrorMsg() << std::endl;
						break;
					}
					_request.setState(_request.isChunked() ? STATE_CHUNKED : STATE_BODY);
				}
				else _request.setState(STATE_COMPLETE);
			}
		}
	}
	else if (bytes == 0) {// client closed connection
		std::cout << "Client closed fd=" << _fd << std::endl;
		_closed = true;
		setState(CLOSING);
	}

	std::cout << CYAN << "[DEBUG] : Finished onReadable. State: " << _state << ", Closed: " << _closed << RESET << std::endl;
}

static std::string	get_cgi_executable(std::string path, Client& client)
{
	const Directives* config = client.getConfig();
	const std::map<std::string, std::string>& handlers = config->getCgiHandlers();

	if (handlers.empty())
		return "";

	size_t dot_pos = path.rfind('.');
	if (dot_pos == std::string::npos)
		return "";

	std::string extension = path.substr(dot_pos);

	std::map<std::string, std::string>::const_iterator it = handlers.find(extension);

	if (it != handlers.end())
	{
		client.getRequest().setIsCgi(true);
		return it->second;
	}

	return "";
}

void	Client::process(EpollManager &poller)
{
	std::cout << "[Response] Target resource: " << _response.getPhysicalPath() << std::endl;
	if (_request.getState() == STATE_COMPLETE) {
		_request.log();
		std::string executable_path = get_cgi_executable(_request.getPath(), *this);
		if (!_request.getIsCgi()) {
			std::cout << BLUE << "Normal Request" << RESET << std::endl;
			_response.generate(_request);
			if (_status_code >= 400) {
				std::cout << "[Client] Error response generated. Code: " << _status_code << std::endl;
				_request.setState(STATE_ERROR);
			}
		}
		else {
			std::cout << BLUE << "CGI Request Detected. Executable: " << executable_path << RESET << std::endl;
			std::string script_fs_path = CgiHandler::prepare(this, poller, _fd);

			if (script_fs_path.empty())
			{
				_response.generateError();
				markWriting();
				poller.set_events(_fd, EPOLLOUT);
				return;
			}
			std::cout << BLUE << "CGI Request. Executable: " << executable_path << ", Script: " << script_fs_path << RESET << std::endl;
			CgiHandler::execute(this, script_fs_path, executable_path, poller);
			return ;
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
	/* Allowed Methods */
	if (!_config->isMethodAllowed(Directives::stringToMethod(_request.getMethod()))) {
		setStatus(405); // Method Not Allowed
		return false;
	}
	/* Content-Length */
	if (_request.hasBody() && _request.getContentLength() > _config->getMaxBodySize()) {
		setStatus(413); // Payload Too Large
		return false;
	}

		/* Path Translation */
		_response.translatePath(_request.getPath());

	return true;
}

void	Client::_setBestConfig()
{
	if (_request.getState() == STATE_ERROR) { _config = &_server->getDirectives(); return; }
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
			if (loc_path.length() > best_len) {
				best_match = &locations[i];
				best_len = loc_path.length();
			}
		}
	}
	_config = (best_match) ? &best_match->getDirectives() : &_server->getDirectives();
	_response.setConfig(_config);
}

void	Client::_serveLargeFile()
{
	if (!_file_stream.is_open())
	{
		_file_stream.open(_response.getPhysicalPath().c_str(), std::ios::binary);
		if (_request.getIsCgi())
			_file_stream.seekg(_cgi_header_size, std::ios::beg);
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

		if (!_response.getPhysicalPath().empty() && _request.getIsCgi())
			std::remove(_response.getPhysicalPath().c_str());

		std::cout << RED << "Finished sending file: " << _response.getPhysicalPath() << RESET << std::endl;
		_response.clearFilePath();
	}
}

void	Client::onWritable()
{
	if (!_response.getHeaderBuffer().empty())
	{
		std::cout << RED << "[DEBUG] : send Headers" << RESET << std::endl;

		std::string& h_buf = _response.getHeaderBuffer();
		ssize_t sent = send(_fd, h_buf.c_str(), h_buf.size(), 0);
		if (sent > 0) h_buf.erase(0, sent);
		if (!h_buf.empty()) return;
	}

	if (!_response.getBuffer().empty())
	{
		std::cout << RED << "[DEBUG] : send body from Buffer" << RESET << std::endl;

		std::string& b_buf = _response.getBuffer();
		ssize_t sent = send(_fd, b_buf.c_str(), b_buf.size(), 0);
		if (sent > 0) b_buf.erase(0, sent);

		if (!b_buf.empty()) return;
	}

	if (!_response.getPhysicalPath().empty())
	{
		std::cout << RED << "[DEBUG] : send body from File" << RESET << std::endl;
		_serveLargeFile();
	}

	if (_response.getHeaderBuffer().empty() && 
		_response.getBuffer().empty() && 
		_response.getPhysicalPath().empty())
	{
		std::cout << RED << "Sending DONE" << RESET << std::endl;
		if (!_request.shouldKeepAlive() || _status_code >= 400)
		{
			_state = CLOSING;
			_closed = true;
		}
		else resetForNextRequest();
	}
}

void	Client::parseCgiResponse()
{
	std::string& raw = _cgi_header_buf;
	size_t split_pos = raw.find("\r\n\r\n");
	size_t sep_len = 4;

	if (split_pos == std::string::npos)
	{
		split_pos = raw.find("\n\n");
		sep_len = 2;
	}

	if (split_pos == std::string::npos)
	{
		std::cerr << "502 Bad Gateway: No header-body separator" << std::endl;
		setStatus(502);
		return;
	}

	std::string headers_part = raw.substr(0, split_pos);
	_cgi_header_size = split_pos + sep_len;

	std::string status_line = "HTTP/1.1 200 OK";
	std::string final_headers = "";

	std::string	line;
	size_t		start = 0;
	size_t		end = headers_part.find('\n');

	while (start < headers_part.length())
	{
		if (end == std::string::npos)
			line = headers_part.substr(start);
		else
			line = headers_part.substr(start, end - start);

		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);

		if (!line.empty())
		{
			if (line.compare(0, 7, "Status:") == 0)
			{
				if (line.size() > 8)
				{
					size_t val_start = (line[7] == ' ') ? 8 : 7;
					status_line = "HTTP/1.1 " + line.substr(val_start);
				}
			} else if (line.find(':') == std::string::npos) {
				std::cerr << "502 Bad Gateway: Malformed header line: " << line << std::endl;
				setStatus(502);
				return ;
			} else
				final_headers += line + "\r\n";
		}

		if (end == std::string::npos)
			break;
		start = end + 1;
		end = headers_part.find('\n', start);
	}

	long total_file_size = Utils::getFileSize(_cgi_temp_path);
	long body_size = total_file_size - _cgi_header_size;

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
	std::string filename = "/tmp/webserv_cgi_" + Utils::generateRandomId();

	int fd = open(filename.c_str(), O_CREAT | O_RDWR | O_EXCL, 0644);
	if (fd == -1)
	{
		_status_code = 500;
		return;
	}

	_cgi_temp_path = filename;
	close(fd);

	_cgi_out_file.open(_cgi_temp_path.c_str(), std::ios::binary);
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
short& Client::getStatus() { return _status_code; }
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

	_request.resetState();
	_response.clear();
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
	std::cout << "🔴 Cleaning up Client fd=" << _fd << std::endl;
	if (_file_stream.is_open())
		_file_stream.close();

	if (_fd != -1)
		close(_fd);
}
