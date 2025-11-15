#pragma once

#include "WebServer.hpp"
#include "Directives.hpp"
#include "Server.hpp"

class	Http
{
	public:
		Http();
		Http(const Http& other);
		~Http();
	
		Directives&			getDirectives();
		const Directives&	getDirectives() const;
		const std::vector<Server>&			getServers() const;

		void	setServers(const std::vector<Server>& servers);
		void	addServer(const Server& server);

	
	private:
		Directives					_directives;
		std::vector<Server>			_servers;
};
