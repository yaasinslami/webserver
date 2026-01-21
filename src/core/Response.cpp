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

	std::string path_on_disk = _translatePath(req.getPath(), loc);
	std::cout << "path_on_disk:\t[" << path_on_disk << "]" << std::endl;

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

std::string	Response::_translatePath(const std::string& reqPath, const Location* loc)
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

			return alias + reminder;
		}
	}

	// 2nd priority: root (root never being empty)

	return loc->getDirectives().getRoot() + reqPath;
}

Response::~Response() {}