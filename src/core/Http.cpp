#include "../../inc/Http.hpp"
#include <vector>

Http::Http() :
	_directives()
{}

Http::~Http() {}

Http::Http(const Http& other)
{
	setServers(other.getServers());
}

Directives&	Http::getDirectives()
{
	return this->_directives;
}

const Directives&	Http::getDirectives() const
{
	return this->_directives;
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
