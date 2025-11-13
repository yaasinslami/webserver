#include "Location.hpp"

/*
** ------------------------------- GETTERS --------------------------------
*/

std::string LocationConfig::getPath() const
{
	return this->_path;
}

std::string LocationConfig::getRoot() const
{
	return this->_root;
}

std::string LocationConfig::getAlias() const
{
	return this->_alias;
}

std::string LocationConfig::getIndex() const
{
	return this->_index;
}

std::map<int, std::string> LocationConfig::getErrorPages() const
{
	return this->_error_pages;
}

std::vector<std::string> LocationConfig::getMethods() const
{
	return this->_allowed_methods;
}

bool LocationConfig::getAutoIndex() const
{
	return this->_autoindex;
}

unsigned long LocationConfig::getMaxBodySize() const
{
	return this->_client_max_body_size;
}

/*
** ------------------------------- SETTERS --------------------------------
*/

void LocationConfig::setPath(const std::string &path)
{
	this->_path = path;
}

void LocationConfig::setRoot(const std::string &root)
{
	this->_root = root;
}

void LocationConfig::setAlias(const std::string &alias)
{
	this->_alias = alias;
}

void LocationConfig::setIndex(const std::string &index)
{
	this->_index = index;
}

void LocationConfig::setErrorPages(const std::map<int, std::string> &error_pages)
{
	this->_error_pages = error_pages;
}

void LocationConfig::setMethods(const std::vector<std::string> &allowed_methods)
{
	this->_allowed_methods = allowed_methods;
}

void LocationConfig::setAutoIndex(const bool &autoindex)
{
	this->_autoindex = autoindex;
}

void LocationConfig::setClientMaxBody(const unsigned long &client_max_body_size)
{
	this->_client_max_body_size = client_max_body_size;
}

LocationConfig::LocationConfig() {}

LocationConfig::~LocationConfig() {}