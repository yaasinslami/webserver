#include "Response.hpp"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include <stdlib.h>

void Response::generate(const Request& req)
{
	if (_config == NULL)
	{
		std::cout << RED << "_config is NULL" << RESET << std::endl;
		return ;
	}

	_translatePath(req.getPath());
	std::cout << "[Response] Target resource: " << _resource_path << std::endl;

	std::string method = req.getMethod();

	// i will remove it use it now just to avoid infinit request send
	if (method == "GET" || method == "POST" || method == "DELETE")
	{
		setCode(501);
		generateError();
		_resource_path.clear();
		return ;
	}
	// if (method == "GET") _handleGet();
	// else if (method == "POST") _handlePost();
	// else if (method == "DELETE") _handleDelete();
}

void Response::_handleGet()
{
	struct stat file_info;
	if (stat(_resource_path.c_str(), &file_info) != 0) { setCode(404); return; }
	if (S_ISDIR(file_info.st_mode)) _handleDirectory();
	// if (S_ISREG(file_info.st_mode)) _serveFile();
}

void	Response::_handleDirectory()
{
	const std::vector<std::string>& index_list = _config->getIndex();

	bool index_found = false;
	for (size_t i = 0; i < index_list.size(); ++i) {
		std::string candidate = _resource_path;
		if (candidate[candidate.length() - 1] != '/') candidate += "/";
		candidate += index_list[i];
		if (access(candidate.c_str(), F_OK) == 0) {
			_resource_path = candidate;
			index_found = true;
			break ;
		}
	}
	if (!index_found) {// i have directory at this point
		if (_config->getAutoIndex()) {// if auto index enbled
			// _generateAutoIndex(); not implimentaton
			std::cout << "auto index : On\tHandling DirectoryListing...\n";
			return ;
		}
		else {// directory listening not allowed
			setCode(403);
			return;
		}
	}
	// stat(_resource_path.c_str(), &file_info);// refreshi tbnmha file_info struct
}

void Response::_translatePath(const std::string uri)
{
	// Plan A: Alias
	std::string alias = _config->getAlias();
	if (!alias.empty()) {
		std::string reminder = uri.substr(_config->getPath().length());

		if (!alias.empty() && alias[alias.length() - 1] != '/' && !reminder.empty() && reminder[0] != '/')
			_resource_path = alias + "/" + reminder;
		else _resource_path = alias + reminder;
		_resource_path = alias + reminder;
	}
	else {// Plan B: Root
		_resource_path = _config->getRoot() + uri;
	}
	_resource_path = _sanitizePath(_resource_path);
}

std::string Response::_sanitizePath(const std::string& path)
{
	std::string sanitized;
	bool last_was_slash = false;

	for (size_t i = 0; i < path.length(); ++i) {
		if (path[i] == '/') {
			if (!last_was_slash) sanitized += path[i];
			last_was_slash = true;
		} else {
			sanitized += path[i];
			last_was_slash = false;
		}
	}
	return sanitized;
}


std::string Response::_getMimeType(const std::string& path)
{
	size_t dot = path.find_last_of(".");
	if (dot == std::string::npos)
		return "application/octet-stream";

	std::string ext = path.substr(dot);
	if (ext == ".html" || ext == ".htm") return "text/html";
	if (ext == ".css") return "text/css";
	if (ext == ".js") return "application/javascript";
	if (ext == ".png") return "image/png";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".gif") return "image/gif";
	if (ext == ".ico") return "image/x-icon";
	if (ext == ".txt") return "text/plain";
	if (ext == ".pdf") return "application/pdf";
	return "application/octet-stream";
}
/* Getters */
std::string&	Response::getBuffer() { return _body_content; }
std::string&	Response::getHeaderBuffer() { return _headers; }
std::string		Response::getFilePath() const { return _resource_path; }
/* Setters */
void Response::setCode(short code) { _code = code; }
void Response::setConfig(const Directives* config) { _config = config; }
void Response::setPath(std::string& path ) { _resource_path = path; }
/* Constructors */
Response::Response(short& code) : _code(code), _config(NULL) {}
Response::Response(const Response& other) : _code(other._code) { *this = other; }
Response& Response::operator=(const Response& other)
{
	if (this != &other) {
		_code = other._code;
	}
	return *this;
}
Response::~Response() {}
/* Errors */
std::string	Response::_getStatusMsg(short code)
{
	switch (code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 504: return "Gateway Timeout";
		default:  return "Internal Server Error";
	}
}
void Response::generateError()
{
	std::string error_body;
	std::string msg = _getStatusMsg(_code);
	bool        custom_page_loaded = false;

	if (_config)
	{
		const std::map<int, std::string>& error_pages = _config->getErrorPages();
		std::map<int, std::string>::const_iterator it = error_pages.find(_code);

		if (it != error_pages.end())
		{
			std::string full_path = Utils::normalize_path(_config->getRoot() + it->second);
			std::ifstream file(full_path.c_str());
			if (file.is_open())
			{
				std::stringstream ss;
				ss << file.rdbuf();
				error_body = ss.str();
				custom_page_loaded = true;
				file.close();
			}
		}
	}

	if (!custom_page_loaded)
	{
		std::stringstream ss;
		ss << "<html>\r\n<head><title>" << _code << " " << msg << "</title></head>\r\n";
		ss << "<body bgcolor=\"white\">\r\n<center><h1>" << _code << " " << msg << "</h1></center>\r\n";
		ss << "<hr><center>webserv</center>\r\n</body>\r\n</html>\r\n";
		error_body = ss.str();
	}

	std::stringstream h_ss;
	h_ss << "HTTP/1.1 " << _code << " " << msg << "\r\n";
	h_ss << "Content-Type: text/html\r\n";
	h_ss << "Content-Length: " << error_body.length() << "\r\n";
	h_ss << "Connection: close\r\n"; 
	h_ss << "\r\n";

	_headers = h_ss.str();
	_body_content = error_body;
}

/* Response reseter */
void	Response::clear()
{
	_headers.clear();
	_body_content.clear();
	_resource_path.clear();
	_config = NULL;
	_code = 200;
}

void	Response::clearFilePath() { _resource_path.clear(); }
