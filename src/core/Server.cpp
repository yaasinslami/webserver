#include "../../inc/Server.hpp"

Server::Server() :
	_port(-1),
	_server_name("")
{}

Server::~Server() {}

std::string	Server::getInterface() const
{
	return this->_interface;
}

int	Server::getPort() const
{
	return this->_port;
}

std::string	Server::getServerName() const
{
	return this->_server_name;
}

std::string	Server::getRoot() const
{
	return this->_root;
}

std::string	Server::getIndex() const
{
	return this->_index;
}

unsigned long	Server::getMaxBodySize() const
{
	return this->_client_max_body_size;
}

std::map<int, std::string>	Server::getErrorPages() const
{
	return this->_error_pages;
}

std::vector<Location>		Server::getLocation() const
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

void Server::setRoot(const std::string &root)
{
	this->_root = root;
}

void Server::setIndex(const std::string &index)
{
	this->_index = index;
}

void Server::setMaxBodySize(const unsigned long &client_max_body_size)
{
	this->_client_max_body_size = client_max_body_size;
}

void Server::setErrorPages(const std::map<int, std::string> &error_pages)
{
	this->_error_pages = error_pages;
}

void Server::setLocation(const std::vector<Location> &locations)
{
	this->_locations = locations;
}
