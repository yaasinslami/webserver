#include "Http.hpp"

Http::Http(){}

Http::~Http() {}

std::string					Http::getRoot() const
{
	return this->_root;
}

std::string					Http::getIndex() const
{
	return this->_index;
}

unsigned long				Http::getMaxBodySize() const
{
	return this->_client_max_body_size;
}

std::map<int, std::string>	Http::getErrorPages() const
{
	return this->_error_pages;
}

std::vector<Server>			Http::getServers() const
{
	return this->_servers;
}

void	Http::setRoot(const std::string& root)
{
	this->_root = root;
}

void	Http::setIndex(const std::string& index)
{
	this->_index = index;
}

void	Http::setMaxBodySize(const unsigned long& client_max_body_size)
{
	this->_client_max_body_size = client_max_body_size;
}

void	Http::setErrorPages(const std::map<int, std::string>& error_pages)
{
	this->_error_pages = error_pages;
}

void	Http::setServers(const std::vector<Server>& servers)
{
	this->_servers = servers;
}

void	Http::addServer(const Server& server)
{
	this->_servers.push_back(server);
}
