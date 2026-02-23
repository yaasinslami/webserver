#include "Response.hpp"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

void Response::generate(const Request& req)
{
	/* Method Dispatch */
	std::string method = req.getMethod();
	if (method == "GET") _handleGet(req.getPath());
	// else if (method == "POST") _handlePost();
	// else if (method == "DELETE") _handleDelete();
}

void Response::_handleGet(const std::string& uri)
{
	struct stat file_info;
	if (stat(_resource_path.c_str(), &file_info) != 0) { setCode(404); return; }
	if (S_ISDIR(file_info.st_mode)) _handleDirectory(uri);
	if (S_ISREG(file_info.st_mode)) _serveFile();
}

void Response::_handleDirectory(const std::string& uri)
{
	const std::string& path = uri;
	if (!path.empty() && path[path.length() - 1] != '/') {
		setCode(301);
		_headers += "HTTP/1.1 301 Moved Permanently\r\n";
		_headers += "Location: " + path + "/\r\n";
		_headers += "Content-Length: 0\r\n\r\n";
		std::cout << "[Response] Redirecting missing trailing slash to: " << path << "/" << std::endl;
		return;
	}

	const std::vector<std::string>& index_list = _config->getIndex();
	for (size_t i = 0; i < index_list.size(); ++i) {
		std::string candidate = _resource_path;

		// Ensure our physical path ends with a slash before appending the index name
		if (candidate[candidate.length() - 1] != '/') {
			candidate += "/";
		}
		candidate += index_list[i];

		// Use R_OK instead of F_OK! 
		// F_OK only checks if it exists. R_OK checks if we actually have permission to read it.
		if (access(candidate.c_str(), R_OK) == 0) {
			
			// We found it! Update the resource path to the exact file.
			_resource_path = candidate;
			
			// Instantly transition to serving this file just like a normal GET request
			_serveFile(); 
			return;
		}
	}

	// 3. Fallback to Autoindex if no index files were found
	if (_config->getAutoIndex()) {
		std::cout << "[Response] Autoindex ON. Generating Directory Listing...\n";
		
		// _generateAutoIndex(); // We will write this next!
		
		// Note: _generateAutoIndex should setCode(200) and build the HTML string
		return;
	}

	// 4. If no index file and no autoindex, it is strictly forbidden.
	std::cout << "[Response] Directory access forbidden (No index, Autoindex OFF).\n";
	setCode(403);
}

void	Response::_serveFile()
{
	struct stat file_stat;
	if (stat(_resource_path.c_str(), &file_stat) != 0) { setCode(404); return; }
	_file_size = file_stat.st_size; 

	_file_fd = open(_resource_path.c_str(), O_RDONLY);
	if (_file_fd < 0) { setCode(403); return; }

	_headers += "HTTP/1.1 200 OK\r\n";
	_headers += "Content-Type: " + Utils::getMimeType(_resource_path) + "\r\n";
	_headers += "Content-Length: " + Utils::toString(_file_size) + "\r\n";
	_headers += "\r\n";
	setCode(200);
	std::cout << "File opened successfully. Prepared to send " << _file_size << " bytes.\n";
}

void Response::translatePath(const std::string& uri)
{
	std::string alias = _config->getAlias();
	if (!alias.empty()) {
		std::string reminder = uri.substr(_config->getPath().length());
		_resource_path = alias + reminder;
	}
	else _resource_path = _config->getRoot() + uri;

	_resource_path = Utils::sanitizePath(_resource_path);
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