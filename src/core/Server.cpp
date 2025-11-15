#include "../../inc/Server.hpp"

Server::Server(const Directives& g_directives) :
	_directives(g_directives),
	_port(-1),
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
	this->_interface = interface;
}

void Server::setPort(const int &port)
{
	this->_port = port;
}

void Server::setServerName(const std::string &server_name)
{
	this->_server_name = server_name;
}

void Server::setLocations(const std::vector<Location> &locations)
{
	this->_locations = locations;
}

void	Server::addLocation(const Location& location)
{
	this->_locations.push_back(location);
}

void	Server::setDirectives(const Directives& directves)
{
	this->_directives = directves;
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
