#pragma once

#include "WebServer.hpp"
#include "Server.hpp"
#include "Location.hpp"

class	Server;

class	Location
{
	public:
		Location(const Directives& server_directives);
		~Location();


		/* Getters */
		std::string					getPath() const;
		std::string					getAlias() const;
		
		/* Setters */
		void						setPath(const std::string& path);
		void						setAlias(const std::string& alias);

	private:
		Directives					_directives;
		std::string					_path;
		std::string					_alias;
};