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

void Location::setAlias(const std::string &alias) { _directives.setAlias(alias); }

void	Location::logs() const
{
	std::cout << "Location [" << _directives.getPath() << "]\n";
	getDirectives().logs();
	std::cout << "\n";
}


bool Location::compareLocations(const Location& a, const Location& b)
{
	std::string path_a = a.getDirectives().getPath();
	std::string path_b = b.getDirectives().getPath();

	if (path_a.length() != path_b.length())
		return path_a.length() > path_b.length();// longest path first

	return path_a < path_b;// alphabetical order if same length
}