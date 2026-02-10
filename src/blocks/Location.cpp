#include "Location.hpp"
#include <iostream>


Location::~Location() {}

Location::Location(const Directives& s_directives) : _directives(s_directives) {}

Directives&	Location::getDirectives() { return this->_directives; }

const Directives&	Location::getDirectives() const { return this->_directives; }

void Location::setPath(const std::string &path)
{
	if (path[0] != '/')
		_directives.setPath("/" + path);
	else
	_directives.setPath(path);
}

void Location::setAlias(const std::string &alias)
{
	_directives.setAlias(alias);
}

void	Location::logs() const
{
	std::cout << "Location [" << _directives.getPath() << "]\n";
	getDirectives().logs();
	std::cout << "\n";
}
