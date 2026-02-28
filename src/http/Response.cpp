#include "Response.hpp"
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdlib.h>

void	Response::generate(const Request& req)
{
	/* Method Dispatch */
	std::string method = req.getMethod();
	if (method == "GET") _handleGet(req.getPath());
	else if (method == "POST") _handlePost(req);
	else if (method == "DELETE") _handleDelete();

	if (_code < 400)
		_buildHeaders();
}

void	Response::_handleDelete()
{
	// we already have the translated path in _resource_path from the initial translation step in Client::_validateRequest()

	std::cout << "[Response] Attempting to DELETE resource: " << _resource_path << "\n";

	if (!Utils::Myaccess(_resource_path, _code, W_OK, true)) {
		std::cout << "[Response] DELETE forbidden: No write permission for " << _resource_path << "\n";
		return;
	}

	if (std::remove(_resource_path.c_str()) == 0) {
		std::cout << "[Response] Successfully deleted: " << _resource_path << "\n";

		// HTTP 204 No Content means the action was successful but there is no response body
		setCode(204);
		_file_size = 0;
		clearFilePath(); 
	} 
	else {
		std::cout << "[Response] DELETE failed: std::remove() error: " << std::strerror(errno) << "\n";
		setCode(500); // Internal Server Error
	}
}

void	Response::_handlePost(const Request& req)
{
	std::cout << "[Response] Handling POST request for: " << _resource_path << "\n";
	std::string content_type = req.getHeader("Content-Type");

	// Block HTML Form envelopes. These belong to CGI!
	if (content_type.find("multipart/form-data") != std::string::npos ||
		content_type.find("application/x-www-form-urlencoded") != std::string::npos) 
	{
		std::cout << "[Response] Form data not supported for static uploads. Use a CGI script.\n";
		setCode(415); // 415 Unsupported Media Type
		return;
	}

	/* now we only support raw binary uploads, which is a bit weird but simple to implement
	static files should be served by GET, and dynamic uploads should be handled by CGI.
	This is just a fallback for clients that want to do a raw POST to a static path. */

	if (!_config->canUpload()) {
		std::cout << "[Response] POST forbidden: Uploads are disabled (upload off).\n";
		setCode(403); // Forbidden
		return;
	}

	std::string upload_dir = _config->getUploadPath();
	if (upload_dir.empty()) {
		std::cout << "[Response] POST failed: Uploads enabled, but no upload_loc provided.\n";
		setCode(500); // Internal Server Error (This is a server misconfiguration!)
		return;
	}

	std::string filename = req.getUploadFilename();
	if (filename.empty()) {
		filename = "upload_" + Utils::toString(time(NULL)) + ".bin";
	}

	std::string target_path = Utils::sanitizePath(_config->getRoot() + upload_dir + "/" + filename);

	_redirect_location = upload_dir + "/" + filename;// we do not expose the full filesystem path in the Location header, just the URI path relative to the server root
	// Special case: If Content-Length is 0 and not chunked, we should still create an empty file if uploads are allowed.
	if (req.getContentLength() == 0 && !req.isChunked())
	{
		std::ofstream empty_file(target_path.c_str(), std::ios::binary | std::ios::trunc);
		if (empty_file.is_open()) {
			empty_file.close();
			std::cout << "[Response] Created empty file: " << target_path << "\n";
			clearFilePath();
			setCode(201); // 201 Created is more accurate since a file was made
		}
		else {
			std::cerr << "[Response] Failed to create empty file (" << std::strerror(errno) << ").\n";
			setCode(500);
		}
		return;
	}

	std::string temp_path = req.getBodyPath();
	if (temp_path.empty()) {
		std::cout << "[Response] POST failed: No body payload received.\n";
		setCode(400); // Bad Request
		return;
	}

	/* To be removed after testing: */
	std::cout << RED << "[Debug] body size: " << req.getBodySize() << RESET << "\n";
	std::cout << "[Debug] Source Temp File: " << temp_path << "\n";
	std::cout << "[Debug] Destination File: " << target_path << "\n";

	if (std::rename(temp_path.c_str(), target_path.c_str()) == 0) {
		std::cout << "[Response] Fast-moved upload to: " << target_path << "\n";
	}
	else {// If rename fails, it could be due to cross-device issues (EXDEV)
		// Fallback: Manual stream copy if cross-device link fails (EXDEV error)

		std::cout << "[Response] rename() failed (" << std::strerror(errno) << "). Falling back to stream copy.\n";

		std::ifstream src(temp_path.c_str(), std::ios::binary);
		std::ofstream dst(target_path.c_str(), std::ios::binary);
		
		if (!src || !dst) {
			std::cerr << "[Error] Failed to open files for POST copy.\n";
			setCode(500); // Internal Server Error
			return;
		}

		dst << src.rdbuf();
		
		src.close();
		dst.close();

		if (std::remove(temp_path.c_str()) != 0)
			std::cerr << "[Warning] Failed to std::remove temp file: " << temp_path << "\n";
		
		std::cout << "[Response] Successfully copied upload to: " << target_path << "\n";
	}

	clearFilePath();
	setCode(201);
}

void	Response::_handleGet(const std::string& uri)
{
	struct stat file_info;
	
	if (!Utils::Myaccess(_resource_path, _code, R_OK, false)) {
		std::cout << "[Response] GET forbidden: No read permission for " << _resource_path << "\n";
		return;
	}

	stat(_resource_path.c_str(), &file_info);

	if (S_ISDIR(file_info.st_mode)) {
		std::cout << "[Response] Resource is a directory. Handling directory logic...\n";
		_handleDirectory(uri);
	}
	else if (S_ISREG(file_info.st_mode)) {
		_serveFile();
	}
	else {
		std::cout << "[Response] Resource is neither a file nor a directory. Forbidden.\n";
		setCode(403);
	}
}

void	Response::_handleDirectory(const std::string& uri)
{
	const std::string& path = uri;
	if (!path.empty() && path[path.length() - 1] != '/') {
		setCode(301);
		_redirect_location = path + "/";
		std::cout << "[Response] Redirecting missing trailing slash to: " << path << "/" << std::endl;
		clearFilePath();
		return;
	}

	const std::vector<std::string>& index_list = _config->getIndex();
	for (size_t i = 0; i < index_list.size(); ++i) {
		std::string candidate = _resource_path;
		if (candidate[candidate.length() - 1] != '/') {
			candidate += "/";
		}
		candidate += index_list[i];

		if (Utils::Myaccess(candidate, _code, R_OK, true)) {
			_resource_path = candidate;
			_serveFile();
			return;
		}
	}
	if (_config->getAutoIndex()) {
		std::cout << "[Response] Autoindex ON. Generating Directory Listing...\n";
		_generateAutoIndex(uri);
		clearFilePath();
		return;
	}

	std::cout << "[Response] Directory access forbidden (No index, Autoindex OFF).\n";
	setCode(403);
}

void	Response::_generateAutoIndex(const std::string& uri)
{
	
	DIR* dir = opendir(_resource_path.c_str());
	if (!dir) {
		std::cerr << "[Error] Failed to open directory for autoindex: " << _resource_path << std::endl;
		setCode(403);
		return;
	}

	_body_content = "<html>\r\n<head><title>Index of " + uri + "</title></head>\r\n";
	_body_content += "<body bgcolor=\"white\">\r\n";
	_body_content += "<h1>Index of " + uri + "</h1><hr><pre>\r\n";

	struct dirent* entry;
	std::stringstream ss;
	while ((entry = readdir(dir)) != NULL) {
		std::string filename = entry->d_name;
		if (filename == ".") continue;

		std::string full_path = _resource_path;
		if (full_path[full_path.length() - 1] != '/') full_path += "/";
		full_path += filename;

		struct stat entry_stat;
		if (stat(full_path.c_str(), &entry_stat) != 0) {
			std::cerr << "[Error] Failed to stat entry for autoindex: " << full_path << std::endl;
			continue;
		}

		bool is_dir = S_ISDIR(entry_stat.st_mode);
		if (is_dir) filename += "/";

		// Format the time "12-May-2023 14:32"
		struct tm* tm_info = localtime(&entry_stat.st_mtime);
		char time_buffer[64];
		strftime(time_buffer, sizeof(time_buffer), "%d-%b-%Y %H:%M", tm_info);

		std::string display_name = filename;
		if (display_name.length() > 50)
			display_name = display_name.substr(0, 47) + "...";

		ss << "<a href=\"" << filename << "\">" << display_name << "</a>";

		int padding = 50 - display_name.length();
		if (padding < 0) padding = 0;
		ss << std::string(padding, ' ');
		ss << std::setw(20) << std::right << time_buffer;

		if (is_dir)
			ss << std::setw(20) << std::right << "-"; // Directories don't show size
		else
			ss << std::setw(20) << std::right << entry_stat.st_size; // File size
		ss << "\r\n";
	}

	closedir(dir);
	_body_content += ss.str();
	_body_content += "</pre><hr></body>\r\n</html>\r\n";

	_content_type = "text/html";
	_is_static_file = false;

	setCode(200);

	std::cout << "[Response] Generated Autoindex for " << uri << " (" << _file_size << " bytes)\n";
}

void	Response::_serveFile()
{
	std::cout << "[Response] Serving file: " << _resource_path << std::endl;

	struct stat file_stat;
	if (stat(_resource_path.c_str(), &file_stat) != 0) {
		setCode(404);
		return;
	}

	_file_size = static_cast<size_t>(file_stat.st_size);
	_content_type = Utils::getMimeType(_resource_path);
	_is_static_file = true;
	setCode(200);

	std::cout << "File opened successfully. Prepared to send " << _file_size << " bytes.\n";
}

void	Response::translatePath(const std::string& uri)
{
	std::string alias = _config->getAlias();
	if (!alias.empty()) {
		std::string reminder = uri.substr(_config->getPath().length());
		_resource_path = alias + reminder;
	}
	else _resource_path = _config->getRoot() + uri;

	_resource_path = Utils::sanitizePath(_resource_path);
}

void	Response::clear()
{
	_file_size = 0;
	_content_type.clear();
	_resource_path.clear();
	_headers.clear();
	_body_content.clear();
	_redirect_location.clear();
	_is_static_file = false;
	_config = NULL;
}

void	Response::_buildHeaders()
{
	_headers = "HTTP/1.1 " + Utils::toString(_code) + " " +  Utils::getStatusMessage(_code) + "\r\n";
	
	// Location header for redirection in case of 301 or 302
	if (!_redirect_location.empty())
		_headers += "Location: " + _redirect_location + "\r\n";
	
	if (_is_static_file) {
		_headers += "Content-Type: " + _content_type + "\r\n";
		_headers += "Content-Length: " + Utils::toString(_file_size) + "\r\n";
	}
	else if (!_body_content.empty()) {
		_headers += "Content-Type: " + _content_type + "\r\n";
		_headers += "Content-Length: " + Utils::toString(_body_content.length()) + "\r\n";
	}

	_headers += "\r\n";
}

/* Getters */
std::string&	Response::getBuffer() { return _body_content; }
std::string&	Response::getHeaderBuffer() { return _headers; }
std::string		Response::getPhysicalPath() const { return _resource_path; }
/* Setters */
void	Response::setCode(short code) { _code = code; }
void	Response::setConfig(const Directives* config) { _config = config; }
void	Response::setPath(std::string& path ) { _resource_path = path; }
/* Constructors */
Response::Response(short& code) : _code(code), _file_size(0), _is_static_file(false),
	_config(NULL)
{}
Response::Response(const Response& other) : _code(other._code) { *this = other; }
Response&	Response::operator=(const Response& other)
{
	if (this != &other) {
		_code = other._code;
	}
	return *this;
}
Response::~Response() {}
/* Errors */
void	Response::generateError()
{
	std::string error_body;
	std::string msg =  Utils::getStatusMessage(_code);
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

	clearFilePath();
}

void	Response::clearFilePath() { _resource_path.clear(); }
