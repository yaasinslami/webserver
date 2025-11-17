#pragma once

#include "Location.hpp"
#include <cstdint>
#include <netinet/in.h>

enum	ServerTypes
{
	INTERFACE,
	PORT,
	S_NAME,
	LOCATION
};

class	Server
{
	public:
		Server(const Directives& g_directives);
		~Server();

		Server& operator=(const Server& other);


		void	initServer( void );

		/* Getters */
		const Directives&				getDirectives() const;
		Directives&						getDirectives();
		const in_addr_t&				getInterface() const;
		const uint16_t&					getPort() const;
		const std::string&				getServerName() const;
		const std::vector<Location>&	getLocation() const;

		/* Setters */
		void						setInterface(const std::string& interface);
		void						setInterface(const in_addr_t& inter);
		void						setPort(const int& port);
		void						setServerName(const std::string& server_name);
		void						setLocations(const std::vector<Location>& locations);
		void						addLocation(const Location& location);	
		void						setDirectives(const Directives& directves);

		void						validate() const;
		void						logs() const;

	private:
		Directives					_directives;
		in_addr_t					_interface;
		in_port_t					_port;
		std::string					_server_name;
		std::vector<Location>		_locations;
		struct sockaddr_in 			_server_sockaddr;
		int							_listen_fd;

		std::map<ServerTypes, int>	_server_table;
};
