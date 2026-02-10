#include <map>
#include <algorithm>
#include <cstdio>

#include <sys/epoll.h>

#include "Server.hpp"
// #include "Client.hpp"

using namespace std;

class EpollManager
{
	private:
		int		epfd;
		int		timeout_ms;
		bool	running;

		map<int, Client*>	clients;
		map<int, Server*>		listeners;

		void	handle_listener_event(int fd);
		void	handle_client_event(int fd, uint32_t events);		
	
	public:
		EpollManager(int timeout = 60000);
		~EpollManager();

		bool	add_listener(Server* srv);
		bool	remove_listener(Server* srv);
		bool	add_client(Client* conn);
		bool	remove_client(int fd);

		void	set_events(int fd, uint32_t events);
		void	run_loop();
		void	stop();
};
