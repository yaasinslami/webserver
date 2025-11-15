#pragma once

#include "WebServer.hpp"
#include "Directives.hpp"
#include "Location.hpp"
//class	Location;

class	Server
{
	public:
		Server(const Directives& g_directives);
		~Server();

		Server& operator=(const Server& other);

		/* Getters */
		const Directives&				getDirectives() const;
		Directives&						getDirectives();
		const std::string&				getInterface() const;
		const int&						getPort() const;
		const std::string&				getServerName() const;
		const std::vector<Location>&	getLocation() const;

		/* Setters */
		void						setInterface(const std::string& interface);
		void						setPort(const int& port);
		void						setServerName(const std::string& server_name);
		void						setLocations(const std::vector<Location>& locations);
		void						addLocation(const Location& location);	
		void						setDirectives(const Directives& directves);

		void						logs() const;

	private:
		Directives					_directives;
		std::string					_interface;
		int							_port;
		std::string					_server_name;
		std::vector<Location>		_locations;
};
