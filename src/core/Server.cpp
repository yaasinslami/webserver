#include "../../inc/Server.hpp"
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>

Server::Server(const Directives& g_directives) :
	_directives(g_directives),
	_interface(0),
	_port(80),
	_server_name("")
{
	_server_table[INTERFACE];
	_server_table[PORT];
	_server_table[S_NAME];
	_server_table[LOCATION];	
}

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

void	Server::initServer()
{
	//if (_listen_fd = socket(AF_INET, SOCK_STREAM, 0));

	// struct	sockaddr	addr;
	// struct sockaddr_in	name;
}

const Directives&	Server::getDirectives() const
{
	return this->_directives;
}

Directives&	Server::getDirectives()
{
	return this->_directives;
}

const in_addr_t&	Server::getInterface() const
{
	return this->_interface;
}

const uint16_t&	Server::getPort() const
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
	this->_interface = inet_addr(interface.c_str());
}

void	Server::setInterface(const in_addr_t& inter)
{
	this->_interface = inter;
}

void Server::setPort(const int &port)
{
	_server_table[PORT]++;
	if (port < 1 || port > 65636)
		throw std::runtime_error("Wrong syntax: port");
	this->_port = (uint16_t) port;
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

	if (_server_table.at(S_NAME) != 1)
		throw std::runtime_error("Missing 'server_name'directve.");

	if (_server_table.at(LOCATION) < 1)
		throw std::runtime_error("Missing a 'location' block.");
}

void	Server::logs() const
{
	std::cout << "Server [" << getServerName() << "]\n";

	getDirectives().logs();
	std::cout << "interface: \"" << getInterface() << "\"\n";
	std::cout << "port: \"" << getPort() << "\"\n";


	for (std::vector<Location>::const_iterator it = _locations.begin(); it != _locations.end(); std::advance(it, 1)) {
		it->logs();
	}
	std::cout << std::endl;
}
