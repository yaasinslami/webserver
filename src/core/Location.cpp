#include "../../inc/Location.hpp"
#include <string>

Location::Location() :
	_alias(""),
{}

Location::~Location() {}

Location::Location(const Server& server)
{
	_directives = server.getDirectives();
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

