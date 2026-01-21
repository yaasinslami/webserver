#include <sys/stat.h>
#include "Response.hpp"

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
		_handleGet(client);
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

	// 2nd priority: root (root never being empty)

	_resource = loc->getDirectives().getRoot() + reqPath;
}

void	Response::_handleGet(const Client& client)
{
	struct stat file_info;

	if (stat(_resource.c_str(), &file_info) != 0) {
		_code = 404;
		return;
	}

	if (S_ISDIR(file_info.st_mode)) {
		// ... directory logic ...
	}

	if (S_ISREG(file_info.st_mode)) {
		_serveFile();
	}
}


void	Response::_serveFile()
{
	std::ifstream file(_resource.c_str(), std::ios::binary);

	
}

Response::~Response() {}