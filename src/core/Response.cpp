#include "Response.hpp"

Response::Response(Client& client) : _code(200)
{
	const Request &req = client.getRequest();
	const Location *loc = client.getLocation();
	// const Server *serv = client.getServer();

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
		// _handleGet(client);
	}
	else if (method == "POST") {
		// _handlePost(client);
	}
	else if (method == "DELETE") {
		// _handleDelete(client);
	}
}

std::string	Response::_translatePath(const std::string& path, const Location* loc)
{
	std::string	alias = loc->getAlias();

	// 1st priority: alias
	if (!alias.empty()) {
		std::string loc_path = loc->getPath();

		if (path.find(loc_path) == 0) {
			std::string	reminder = path.substr(loc_path.length());

			return alias + reminder;
		}
	}

	// 2nd priority: root
	return loc->getDirectives().getRoot() + path;
}

Response::~Response() {}