#include "../../inc/Http.hpp"

Http::Http() :
	// _root("/www/html"),
	// _index("index.html"),
	// _client_max_body_size(1048576)
	_directives()
{}

Http::~Http() {}

Http::Http(const Http& other)
{
	setServers(other.getServers());
}


const std::vector<Server>&	Http::getServers() const
{
	return this->_servers;
}

void	Http::setServers(const std::vector<Server>& servers)
{
	this->_servers = servers;
}

void	Http::addServer(const Server& server)
{
	this->_servers.push_back(server);
}
