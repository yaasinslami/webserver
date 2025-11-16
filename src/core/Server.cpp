#include "../../inc/Server.hpp"
#include <stdexcept>

Server::Server(const Directives& g_directives) :
	_directives(g_directives),
	_interface("0.0.0.0"),
	_port(80),
	_server_name("")
{}

Server::~Server() {}

Server& Server::operator=(const Server& other)
{
	if (this != &other)
	{
		setDirectives(other.getDirectives());
		setInterface(other.getInterface());
		setPort(other.getPort());
		setServerName(other.getServerName());
		setLocations(other.getLocation());
	}
	return *this;
}

const Directives&	Server::getDirectives() const
{
	return this->_directives;
}

Directives&	Server::getDirectives()
{
	return this->_directives;
}

const std::string&	Server::getInterface() const
{
	return this->_interface;
}

const int&	Server::getPort() const
{
	return this->_port;
}

const std::string&	Server::getServerName() const
{
	return this->_server_name;
}

const std::vector<Location>&		Server::getLocation() const
{
	return this->_locations;
}

void Server::setInterface(const std::string &interface)
{
	_server_table[INTERFACE]++;
	this->_interface = interface;
}

void Server::setPort(const int &port)
{
	_server_table[PORT]++;
	this->_port = port;
}

void Server::setServerName(const std::string &server_name)
{
	_server_table[S_NAME]++;
	this->_server_name = server_name;
}

void Server::setLocations(const std::vector<Location> &locations)
{
	this->_locations = locations;
}

void	Server::addLocation(const Location& location)
{
	_server_table[LOCATION]++;
	this->_locations.push_back(location);
}

void	Server::setDirectives(const Directives& directves)
{
	this->_directives = directves;
}

void	Server::validate() const
{
	if (_server_table.at(INTERFACE) == 0 && _server_table.at(PORT) == 0)
		throw std::runtime_error("Missing 'listen' directve in server block.");

	if (_server_table.at(INTERFACE) > 1 || _server_table.at(PORT) > 1)
		throw std::runtime_error("Duplicate 'Interface|port' in server block.");

	// if (_server)

}

void	Server::logs() const
{
	std::cout << "Server [" << getServerName() << "]\n";

	getDirectives().logs();
	std::cout << "interface: \"" << getInterface() << "\"\n";
	std::cout << "port: \"" << getPort() << "\"\n";


	for (const auto& location : getLocation()) {
		location.logs();
	}
	std::cout << std::endl;
}
