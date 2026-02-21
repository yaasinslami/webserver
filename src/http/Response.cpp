#include "Response.hpp"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

void Response::generate(const Request& req)
{
	if (_config == NULL)
	{
		std::cout << RED << "_config is NULL" << RESET << std::endl;
		return ;
	}

	_translatePath(req.getPath());
	std::cout << "[Response] Target resource: " << _resource_path << std::endl;

	std::string method = req.getPath();
	if (method == "GET") _handleGet();
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

void Response::setCode(short code) { _code = code; }
void Response::setConfig(const Directives* config) { _config = config; }
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
// void	Response::generateError(const Directives* config) {}