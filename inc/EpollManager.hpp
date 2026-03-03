#pragma once

#include "Client.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <cerrno>

#include <sys/epoll.h>
#include <sys/wait.h>

#define MAX_EVENTS 128
#define SIZE_8KB 8192 

class Client; 
class Server;

class EpollManager
{
	private:
		int		epfd;
		int		timeout_ms;

		std::map<int, Client*>					clients;
		std::map<int, std::vector<Server*> >	listeners;
		std::map<int, Client*>					_cgi_fd_map;
	public:
		/* Constructors */
		EpollManager(std::vector<Server>&);
		~EpollManager();

		bool	add_listener(Server* srv);
		bool	remove_listener(Server* srv);

		bool	add_client(Client* conn);
		bool	remove_client(int fd);

		bool	add_cgi_fds(Client* client);
		void	remove_cgi_fds(Client* client, bool send_error);

		void	set_events(int fd, uint32_t events);
		void	run_loop();
		void	stop();

		void	check_client_timeouts();
	private:
		void	handle_listener_event(int fd);
		void	handle_client_event(int fd, uint32_t events);
		void	handle_cgi_event(int fd, uint32_t events);
		bool	_setup_servers(std::vector<Server>& servers);
};
