#include "../../inc/Location.hpp"
#include <string>

bool	Location::isMethodAllowed(const std::string& method) const
{
	return _directives.isMethodAllowed(method);
}

Location::~Location() {}

Location::Location(const Directives& s_directives) :
	_directives(s_directives),
	_path(""),
	_alias("")
{}

Directives&	Location::getDirectives()
{
	return this->_directives;
}

const Directives&	Location::getDirectives() const
{
	return this->_directives;
}


std::string Location::getPath() const
{
	return this->_path;
}

std::string Location::getAlias() const
{
	return this->_alias;
}

void Location::setPath(const std::string &path)
{
	this->_path = path;
}

void Location::setAlias(const std::string &alias)
{
	this->_alias = alias;
}

void	Location::logs() const
{
	std::cout << "Location [" << getPath() << "]\n";

	getDirectives().logs();
	std::cout << "path: \"" << getPath() << "\"\n";
	std::cout << "alias: \"" << getAlias() << "\"\n";

	std::cout << std::endl;
}
