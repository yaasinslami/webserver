#include <sys/stat.h>
#include "Response.hpp"

Response::~Response() {}
Response::Response(Client& client) : _code(200)
{
	const Request &req = client.getRequest();
	const Location *loc = client.getLocation();
	// const Server *serv = client.getServer();0

	// allowed methods
	if (!loc->isMethodAllowed(req.getMethod()))
	{
		_code = 405;
		std::cout << "Error 405: Method " << req.getMethod() << " not allowed.\n";
		return ;
	}

	// max body size
	size_t limit = loc->getDirectives().getMaxBodySize();
	if (limit > 0 && req.getBody().length() > limit)
	{
		_code = 413;
		std::cout << "Error 413: Body size " << req.getBody().length() << " > " << limit << "\n";
		return;
	}

	_translatePath(req.getPath(), loc);
	std::cout << "-> Resource Path:\t[" << _resource << "]" << std::endl;

	_processMethod(client);
}

void Response::_processMethod(Client &client)
{
	std::string method = client.getRequest().getMethod();

	if (method == "GET") {
		_handleGet();
	}
	else if (method == "POST") {
		// _handlePost(client);
	}
	else if (method == "DELETE") {
		// _handleDelete(client);
	}
}

void	Response::_translatePath(const std::string& reqPath, const Location* loc)
{
	// reqPath can be : /home - www/home
	std::string	alias = loc->getAlias();

	std::cout << "alias:[" << alias << "]" << std::endl;

	// 1st priority: alias
	if (!alias.empty()) {
		std::cout << "\tAlias found. Using alias....\n";
		std::string loc_path = loc->getPath();

		if (reqPath.find(loc_path) == 0) {
			std::string	reminder = reqPath.substr(loc_path.length());

			_resource = alias + reminder;
		}
	}

	// 2nd priority: root (Root can not be empty)
	_resource = loc->getDirectives().getRoot() + reqPath;
}

void	Response::_handleGet( void )
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
		// _buildErrorPage();
	}

	// Request Line (Version + Status Code)
	_response_content = "HTTP/1.1 " + _toString(_code) + " " + _getStatusMsg(_code) + "\r\n";

	// /* Headers */

	std::map<std::string, std::string>::iterator	it;
	for (it = _headers.begin(); it != _headers.end(); std::advance(it, 1)) {
		_response_content += it->first + ": " + it->second + "\r\n";
	}

	_response_content += "\r\n";// headers are done - Body Begin

	_response_content += _body;
	std::cout << "--------- Response Content -----------\n";
	std::cout  << _response_content << std::endl;
	std::cout << "---------------------------------------\n";
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
