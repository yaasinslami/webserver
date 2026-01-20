#include "Response.hpp"

Response::Response(Client& client) : _code(200)
{
	const Request &req = client.getRequest();
	const Location *loc = client.getLocation();
	const Server *serv = client.getServer();

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

	_processMethod(client);
}

void Response::_processMethod(Client &client)
{
	std::string method = client.getRequest().getMethod();

	if (method == "GET") {
		_handleGet(client);
	}
	else if (method == "POST") {
		_handlePost(client);
	}
	else if (method == "DELETE") {
		_handleDelete(client);
	}
}