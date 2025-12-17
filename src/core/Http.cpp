#include "../../inc/Http.hpp"
#include <vector>

Http::Http() :
	_directives()
{}

Http::~Http() {}

Http::Http(const Http& other)
{
	*this = other;
}

Http&	Http::operator=(const Http& other)
{
	if (this != &other)
	{
		setDirective(other.getDirectives());
		setServers(other.getServers());
	}
	return *this;
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

void	Http::setDirective(const Directives& directves)
{
	this->_directives = directves;
}

void	Http::setServers(const std::vector<Server>& servers)
{
	this->_servers = servers;
}

void	Http::addServer(const Server& server)
{
	this->_servers.push_back(server);
}

void	Http::logs() const
{
	std::cout << "----------------------------------\n";

	getDirectives().logs();

	for (std::vector<Server>::const_iterator it = _servers.begin(); it != _servers.end(); std::advance(it, 1)) {
		it->logs();
	}
}

void	Http::setup_servers( void )
{
	for (size_t i = 0; i < _servers.size(); ++i)
	{
		_servers[i].init_server();

		_servers_by_port[_servers[i].get_sockaddr().sin_port].push_back(&_servers[i]);
	}
}
