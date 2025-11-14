#pragma once

// #include "WebServer.hpp"
#include "Server.hpp"
#include "Directives.hpp"

class	Http
{
	public:
		Http();
		Http(const Http& other);
		~Http();
	
		const std::vector<Server>&			getServers() const;

		void	setServers(const std::vector<Server>& servers);
		void	addServer(const Server& server);
	
	private:
		Directives					_directives;
		std::vector<Server>			_servers;
};
