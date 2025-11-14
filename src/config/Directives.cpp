#include "../../inc/Directives.hpp"

Directives::Directives() {}

Directives::~Directives() {}

Directives& Directives::operator=(const Directives& dir)
{
	if (this != &dir)
	{
		setRoot(dir.getRoot());
		setIndex(dir.getIndex());
		setAutoIndex(dir.getAutoIndex());
		setAllowedMethods(dir.getAllowedMethods());
		setMaxBodySize(dir.getMaxBodySize());
		setErrorPages(dir.getErrorPages());
	}
}

const std::string&	Directives::getRoot() const
{
	return this->_root;
}

const std::string&	Directives::getIndex() const
{
	return this->_index;
}

const bool&	Directives::getAutoIndex() const
{
	return this->_autoindex;
}

const std::vector<std::string>&	Directives::getAllowedMethods() const
{
	return this->_allowed_methods;
}

const unsigned long&	Directives::getMaxBodySize() const
{
	return this->_client_max_body_size;
}

const std::map<int, std::string>&	Directives::getErrorPages() const
{
	return this->_error_pages;
}

void	Directives::setRoot(const std::string& root)
{
	this->_root = root;
}

void	Directives::setIndex(const std::string& index)
{
	this->_index = index;
}

void	Directives::setAutoIndex(const bool& autoindex)
{
	this->_autoindex = autoindex;
}

void	Directives::setAllowedMethods(const std::vector<std::string>& allowed_methods)
{
	this->_allowed_methods = allowed_methods;
}

void	Directives::addMethod(const std::string& method)
{
	this->_allowed_methods.push_back(method);
}

void	Directives::setMaxBodySize(const unsigned long& client_max_body_size)
{
	this->_client_max_body_size = client_max_body_size;
}

void	Directives::setErrorPages(const std::map<int, std::string>& error_pages)
{
	this->_error_pages = error_pages;
}
