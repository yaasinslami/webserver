#include "../../inc/Location.hpp"
#include <string>

bool	Location::isMethodAllowed(const std::string& method) const
{
	return _directives.isMethodAllowed(method);
}

Location::~Location() {}

Location::Location(const Directives& s_directives) : _directives(s_directives) {}

Directives&	Location::getDirectives() { return this->_directives; }

const Directives&	Location::getDirectives() const { return this->_directives; }


// std::string Location::getPath() const
// {
// 	return this->_path;
// }

// std::string Location::getAlias() const
// {
// 	return this->_alias;
// }

void Location::setPath(const std::string &path)
{
	if (path[0] != '/')
		_directives.setPath("/" + path);
		// _path = "/" + path;
	else
	_directives.setPath(path);
		// _path = path;
}

void Location::setAlias(const std::string &alias)
{
	_directives.setAlias(alias);
	// this->_alias = alias;
}

void	Location::logs() const
{
	std::cout << "Location [" << _directives.getPath() << "]\n";
	getDirectives().logs();
	std::cout << "\n";
}
