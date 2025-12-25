#pragma once

#include "Directives.hpp"
#include "Server.hpp"
#include "Client.hpp"

class	Http
{
	public:
		Http();
		Http(const Http& other);
		~Http();

		Http& operator=(const Http& other);

		Directives&			getDirectives();
		const Directives&	getDirectives() const;
		const std::vector<Server>&			getServers() const;

		void	setDirective(const Directives& directves);
		void	setServers(const std::vector<Server>& servers);
		void	addServer(const Server& server);

		void	logs( void ) const;

		void	setup_servers( void );
		void	run( void );
	
	private:
		Directives					_directives;
		std::vector<Server>			_servers;
		std::map<unsigned short, std::vector<Server*> > _servers_by_port;
		std::vector<struct pollfd>  _poll_fds;
		std::map<int, Client>		_clients;

		void	addto_poll(int fd, short events);
		bool	isListeningSocket(int fd);
		void	handle_new_connection(int);
		void	handle_client_request(int);
		void	remove_from_poll(int);
		void	close_connection(int);
};
