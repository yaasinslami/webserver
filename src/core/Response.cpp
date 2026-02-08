#include <sys/stat.h>
#include "Response.hpp"

Response::~Response() {}
Response::Response(Client& client) : _client(client), _status_code(client.getStatus())
{
	// if it already (>= 400 -> just) return

	const Location* location = _client.getLocation();
	const Server* server   = _client.getServer();
	const Request &req = _client.getRequest();// can be incompleted

	// allowed methods
	if (location && !location->isMethodAllowed(req.getMethod())) {
		_status_code = 405;
		std::cout << "Error 405: Method " << req.getMethod() << " not allowed.\n";
		return ;
	}
	// if it NULL no matched Location ?

	// max body size
	size_t limit = (location != NULL) ? location->getDirectives().getMaxBodySize() : server->getDirectives().getMaxBodySize();
	if (req.getBodySize() > limit) {
		_status_code = 413;
		return;
	}

	_translatePath();
	std::cout << "-> Resource Path:\t[" << _resource << "]" << std::endl;

	// redirecting happen here

	_processMethod();
}

void Response::_processMethod()
{
	std::string method = _client.getRequest().getMethod();

	if (method == "GET") {
		_handleGet();
	}
	else if (method == "POST") {
		_handlePost();
	}
 	else if (method == "DELETE") {
		// _handleDelete();
	}
}

void	Response::_translatePath()
{
	const Location* location = _client.getLocation();
	const Server* server   = _client.getServer();
	const Request &req = _client.getRequest();

	std::string	req_path = req.getPath();

	// ALIAS (Location Only)
	if (location != NULL && !location->getAlias().empty()) {
		std::string alias = location->getAlias();
		std::string	loc_path = location->getPath();

		if (req_path.find(loc_path) == 0) {
			std::string remainder = req_path.substr(loc_path.length());

			// 2. Prepend alias -> "/var/www/photos/cat.png"
			_resource = alias + remainder;

			std::cout << "-> Using Alias: " << _resource << std::endl;
			return ;
		}
	}

	std::string	root;
	if (location != NULL && !location->getDirectives().getRoot().empty()) {
		root = location->getDirectives().getRoot();
	}
	else {
		root = server->getDirectives().getRoot();
	}

	_resource = root + req_path;
}

void	Response::_handleGet()
{
	struct stat file_info;

	if (stat(_resource.c_str(), &file_info) != 0) {
		std::cerr << "Ma file Ma ta 9alwa\t[" << _resource.c_str() << "]\n";
		_code = 404;
		return;
	}

	if (S_ISDIR(file_info.st_mode)) {
		// ... directory logic ...
		std::cout << "\t🚫🚫🚫Is A Directory\n";
		const std::vector<std::string>& index_list = _client.getLocation()->getDirectives().getIndex();

		bool index_found = false;
		for (size_t i = 0; i < index_list.size(); ++i) {
			std::string candidate = _resource;

			if (candidate[candidate.length() - 1] != '/') candidate += "/";

			candidate += index_list[i];

			if (access(candidate.c_str(), F_OK) == 0) {
				_resource = candidate;

				index_found = true;
				break ;
			}
		}

		if (!index_found) {// we have directory at this point
			if (_client.getLocation()->getDirectives().getAutoIndex()) {// if auto index enbled
				// _generateAutoIndex();
				std::cout << "auto index : On\tHandling DirectoryListing...\n";
				return ;
			}
			else {// directory listening not allowed
				_code = 403;
				return;
			}
		}
		stat(_resource.c_str(), &file_info);
	}

	if (S_ISREG(file_info.st_mode)) {
		_serveFile();
	}
}

void	Response::_serveFile()
{
	std::ifstream file(_resource.c_str(), std::ios::binary);

	if (!file.is_open()) {
		_code = 403;
		std::cerr << "Error: Permission denied or IO error for " << _resource << std::endl;
		return;
	}

	std::ostringstream ss;
	ss << file.rdbuf();
	_body = ss.str();

	file.close();// aywa endak tghlet

	_code = 200;
	_headers["Content-Type"] = _getMimeType(_resource);
	_headers["Content-Length"] = _toString(_body.length());
}

void	Response::generateResponse()
{
	if (_code >= 400 && _body.empty()) {
		_buildErrorPage();// bad request also !
	}

	// Request Line (Version + Status Code)
	_response_content = "HTTP/1.1 " + _toString(_code) + " " + _getStatusMsg(_code) + "\r\n";

	// /* Headers */
	std::map<std::string, std::string>::iterator	it;
	for (it = _headers.begin(); it != _headers.end(); std::advance(it, 1)) {
		_response_content += it->first + ": " + it->second + "\r\n";
	}

	_response_content += "\r\n";// mark the end of headers
	_response_content += _body;
}

void	Response::_buildErrorPage()
{
	// get the error_pages map
	const std::map<int, std::string>& error_pages = _client.getLocation()->getDirectives().getErrorPages();

	// we have one code, we excpect the config provide to us an error page path
	if (error_pages.find(_code) != error_pages.end()) {
		std::string error_page = error_pages.at(_code);

		std::string root = _client.getLocation()->getDirectives().getRoot();

		// Logic: "www" + "/404.html" -> "www/404.html"
		std::string final_path = root + error_page;

		std::cout << "Error Page Given:\t[" << error_page << "]\n";

		// handle the path here (path still not trusted)
		std::ifstream file(final_path.c_str(), std::ios::binary);
		if (file.is_open()) {// Hadche li Bghina
			std::ostringstream ss;
			ss << file.rdbuf();
			_body = ss.str();

			// Set headers for the file we just read
			_headers["Content-Type"] = "text/html";
			_headers["Content-Length"] = _toString(_body.length());

			std::cout << "Custom Error Page Served: [" << final_path << "]\n";
			return;
		}
		else {
			std::cerr << "Error: error_page Failed to Open. Using Fallback Error Page\n";
		}
	}
	std::cout << "using fallback error page\n";
	// FallBack Error Page
	std::string color = "#ff6b6b";
	std::string title = _toString(_code) + " " + _getStatusMsg(_code);
	std::string message = "An error occurred.";

	// Optional: Customize message based on code
	if (_code == 404)
		message = "The requested resource was not found.";
	else if (_code == 403)
		message = "Access to this resource is denied.";

	std::ostringstream ss;
	ss << "<!DOCTYPE html><html><head><title>" << title << "</title>"
	   << "<style>body{font-family: sans-serif; text-align: center; padding: 50px; background: #222; color: #fff;}"
	   << "h1{color:" << color << "; font-size: 3em; margin-bottom: 10px;}"
	   << "p{color:#aaa; font-size: 1.2em;}</style></head>"
	   << "<body>"
	   << "<h1>" << _code << "</h1>"
	   << "<h2>" << _getStatusMsg(_code) << "</h2>"
	   << "<p>" << message << "</p>"
	   << "</body></html>";

	_body = ss.str();
	_headers["Content-Type"] = "text/html";
	_headers["Content-Length"] = _toString(_body.length());
}

std::string	Response::_getMimeType(std::string& path)
{
	size_t	dotPos = path.find_last_of('.');

	if (dotPos == std::string::npos) {
		return "application/octet-stream";
	}

	std::string	type = path.substr(dotPos + 1);

	if (type == "html" || type == "htm") return "text/html";
	if (type == "css") return "text/css";
	if (type == "js")  return "application/javascript";
	if (type == "jpg" || type == "jpeg") return "image/jpeg";
	if (type == "png") return "image/png";
	if (type == "gif") return "image/gif";
	if (type == "ico") return "image/x-icon";
	if (type == "txt") return "text/plain";
	if (type == "pdf") return "application/pdf";
	if (type == "mp4") return "video/mp4";

	return "application/octet-stream";
}

std::string	Response::_toString(int value) {
	std::ostringstream os;
	os << value;
	return os.str();	
}

std::string	Response::_getStatusMsg(int code)
{
	switch (code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";

		case 301: return "Moved Permanently";
		case 302: return "Found";

		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";

		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";

		default:  return "Unknown Status";
	}
}

const std::string& Response::getResponseContent() const
{
	return _response_content;
}

std::string	Response::_extractFilename(std::string& headers)
{
	// Content-Type
	// Content-Disposition

	std::string filename = "";
	std::string search = "filename=\"";

	size_t pos = headers.find(search);
	if (pos != std::string::npos) {
		pos += search.length();
		size_t end_pos = headers.find("\"", pos);
		if (end_pos != std::string::npos) {
			filename = headers.substr(pos, end_pos - pos);
		}
	}
	return filename;
}

void	Response::_handlePost()
{
	// check the client_max_body_size here

	// so i think here i should distunguish between the Content-types logic (application/x-www-form-urlencoded or multipart/form-data ....)

	std::string content_type = _client.getRequest().getHeader("content-type");
	std::cout << "\nContent-Type:\t[" << content_type << "]\n\n";

	if (content_type.find("multipart/form-data") != std::string::npos) {
		if (_client.getLocation()->getDirectives().canUpload() == false) {
			_code = 403;
			return ;
		}
		_handleMultiPart();
	}
	// else: It is a Login Form / CGI (x-www-form-urlencoded)
}


void	Response::_handleMultiPart()
{
	// RULE: "--" + BOUNDARY
	// read the bondary from the Header (parse)
	std::string body = _client.getRequest().getBody();
	std::string content_type = _client.getRequest().getHeader("content-type");

	size_t boundary_pos = content_type.find("boundary=");
	if (boundary_pos == std::string::npos) {
		std::cerr << "can't find the boundary\n";
		_code = 400;
		return ;
	}

	std::string	boundary = "--" + content_type.substr(boundary_pos + 9);

	size_t start_pos = body.find(boundary);
	if (start_pos == std::string::npos) {
		_code = 400;
		return ;
	}

	start_pos += boundary.length() + 2;

	// headers end (\r\n\r\n)
	size_t headers_end = body.find("\r\n\r\n", start_pos);
	if (headers_end == std::string::npos) {
		_code = 400; return ;
	}

	std::string part_headers = body.substr(start_pos, headers_end - start_pos);
	std::string filename = _extractFilename(part_headers);

	// If no filename found (e.g. it's just a text field), generate one
	if (filename.empty()) filename = "upload_" + _toString(clock()) + ".bin";

	size_t data_start = headers_end + 4;

	size_t data_end = body.find(boundary, data_start);
	if (data_end == std::string::npos) {
		_code = 400; return;
	}
	data_end -= 2;

	std::string file_content = body.substr(data_start, data_end - data_start);
	_saveFile(filename, file_content);
}

void	Response::_saveFile(std::string& filename, std::string& payload)
{
	std::string upload_path = _client.getLocation()->getDirectives().getRoot() + _client.getLocation()->getDirectives().getUploadPath();
	std::cout << "upload_path=[" << upload_path << "\n\n\n";
	std::string full_path = upload_path + "/" + filename;
	std::cout << "full_path=[" << full_path << "\n\n\n";

	std::ofstream outfile(full_path.c_str(), std::ios::binary);

	if (outfile.is_open()) {
		outfile << payload;// Buffer Copying here
		outfile.close();
		
		_code = 201;
		_body = "<!DOCTYPE html><html><body><h1>File Uploaded Successfully!</h1></body></html>";
		_headers["Location"] = full_path;
		_headers["Content-Type"] = "text/html";
	}
	else {
		_code = 500;
		std::cerr << "Error: Could not save file to " << full_path << std::endl;
	}
}
