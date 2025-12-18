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


		void	init_server( void );

		/* Getters */
		const Directives&				getDirectives() const;
		Directives&						getDirectives();
		const std::string&				getInterfaceStr() const;
		const std::string&					getPortStr() const;
		const std::string&				getServerName() const;
		const std::vector<Location>&	getLocation() const;
		struct sockaddr_in&				get_sockaddr();

		/* Setters */
		void						setInterface(const std::string& interface);
		// void						setInterface(const std::string& inter);
		void						setPort(const std::string& port);
		void						setServerName(const std::string& server_name);
		void						setLocations(const std::vector<Location>& locations);
		void						addLocation(const Location& location);	
		void						setDirectives(const Directives& directves);
		void						set_sockaddr(const struct sockaddr_in&);
		void						set_fd(int fd);

		void						validate() const;
		void						logs() const;

	private:
		Directives					_directives;
		std::string					_interface;
		std::string					_port;
		std::string					_server_name;
		std::vector<Location>		_locations;

		struct sockaddr_in 			_server_sockaddr;
		int							_listen_fd;

		std::map<ServerTypes, int>	_server_table;
};
