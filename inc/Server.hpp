#pragma once

#include "Location.hpp"
#include "Utils.hpp"

#include <vector>
#include <map>
#include <cstdlib>
#include <cerrno>
#include <cstring>


#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>


enum	ServerTypes
{
	INTERFACE,
	PORT,
	S_NAME,
	LOCATION
};

#define BACKLOG 10

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
		const std::string&				getPortStr() const;
		const std::vector<std::string>&	getServerNames() const;
		const std::vector<Location>&	getLocation() const;
		int								get_fd( void );

		/* Setters */
		void						setInterface(const std::string& interface);
		void						setPort(const std::string& port);
		void						setServerName(const std::vector<std::string>& server_name);
		void						setLocations(const std::vector<Location>& locations);
		void						addLocation(const Location& location);	
		void						setDirectives(const Directives& directves);
		void						set_fd(int fd);

		void						addServerName(const std::string& server_name);

		void						validate() const;
		void						logs() const;

		bool						init();
		int							accept_client();


	private:
		Directives					_directives;
		std::string					_interface;
		std::string					_port;
		std::vector<std::string>	_server_name;
		std::vector<Location>		_locations;
		int							_listen_fd;
		std::map<ServerTypes, int>	_server_table;
};
