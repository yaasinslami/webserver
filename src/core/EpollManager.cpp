#include "EpollManager.hpp"

extern volatile sig_atomic_t g_running;

static std::string get_cgi_executable(std::string, Client&);

void	EpollManager::handle_cgi_event(int fd, uint32_t events)
{
	Client* client = _cgi_fd_map[fd];
	if (!client) return;

	client->updateTime();

	if (events & EPOLLERR)
	{
		std::cerr << "⚠️ CGI Pipe Error on fd=" << fd << std::endl;
		remove_cgi_fds(client, true);
		return;
	}

	if (fd == client->getCgiOutPipe() && (events & (EPOLLIN | EPOLLHUP)))
	{
		char buf[BUFFER_SIZE];
		ssize_t bytes = read(fd, buf, sizeof(buf));

		if (bytes > 0)
		{
			if (!client->isCgiFileOpen())
			{
				client->openCgiTempFile();
				if (client->getStatus() == 500)
				{
					std::cout << RED << "Failed to open temp file for CGI output buffering." << RESET << std::endl;
					remove_cgi_fds(client, true);
					client->getResponse().generateError();
					client->setState(WRITING);
					return;
				}
			}

			client->writeToCgiTempFile(buf, bytes);

			if (client->getCgiHeaderBuf().size() < SIZE_8KB)
				client->getCgiHeaderBuf().append(buf, bytes);
		}
		else if (bytes == 0)
		{
			std::cout << GREEN << "CGI output pipe closed by child process (EOF)" << RESET << std::endl;

			remove_cgi_fds(client, false);
			client->closeCgiOutFile();

			if (client->getStatus() != 500)
			{
				client->parseCgiResponse();
				if (client->getStatus() >= 400)
					client->getResponse().generateError();
				client->markWriting();
				set_events(client->getFd(), EPOLLOUT);
			}
		}
	}
}

bool	EpollManager::add_listener(Server *srv)
{
	if (!srv) return false;

	int fd = srv->get_fd();
	if (fd < 0) return false;

	if (listeners.find(fd) == listeners.end()) {
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = fd;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
			return false;
		}
		std::cout << "[Epoll] Listening on new fd=" << fd << std::endl;
	}
	else
	{
		std::vector<Server *> &existing_servers = listeners[fd];
		for (size_t i = 0; i < existing_servers.size(); ++i) {
			if (srv->hasIdenticalNames(existing_servers[i])) {// tkhwira
				std::cerr << "⚠️  WARNING: Conflicting server_name on port [" << srv->getPortStr()
						<< "] (ignored)." << std::endl;
				return false;
			}
		}
		std::cout << "[Epoll] Adding virtual host to existing fd=" << fd << std::endl;
	}
	listeners[fd].push_back(srv);
	return true;
}

bool	EpollManager::remove_listener(Server* srv)
{
	if (!srv)
		return (false);
	
	int fd = srv->get_fd();
	if (!listeners.count(fd))
		return (false);

	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	listeners.erase(fd);

	return (true);
}

bool	EpollManager::add_client(Client* conn)
{
	if (!conn)
		return (false);

	int fd = conn->getFd();
	if (fd < 0 || clients.count(fd))
		return (false);

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
	{
		delete conn;
		return (false);
	}

	clients[fd] = conn;

	std::cout << "[Epoll] New client fd=" << fd << std::endl;
	return (true);
}

bool	EpollManager::remove_client(int fd)
{
	if (!clients.count(fd))
		return (false);

	Client *client = clients[fd];
	if (client->getCgiOutPipe() != -1)
		remove_cgi_fds(client, false);

	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

	delete clients[fd];
	clients.erase(fd);

	return (true);
}

bool	EpollManager::add_cgi_fds(Client *client)
{
	int out_pipe = client->getCgiOutPipe();

	struct epoll_event ev;

	ev.data.fd = out_pipe;
	ev.events = EPOLLIN;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, out_pipe, &ev) == -1)
		return (false);
	
	_cgi_fd_map[out_pipe] = client;

	return true;
}

void EpollManager::remove_cgi_fds(Client *client, bool send_error)
{
	int read_fd = client->getCgiOutPipe();
	pid_t pid = client->getCgiPid();

	if (read_fd != -1)
	{
		epoll_ctl(epfd, EPOLL_CTL_DEL, read_fd, NULL);
		close(read_fd);
		_cgi_fd_map.erase(read_fd);
	}

	client->setCgiOutPipe(-1);

	if (pid != -1)
	{
		int status;
		pid_t result = waitpid(pid, &status, WNOHANG);

		if (result == 0) // Process exited naturally
		{
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0);
		}
		else if (result == pid)
		{
			if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			{
				std::cout << RED << "CGI process exited with error code " << WEXITSTATUS(status) << "." << RESET << std::endl;
				send_error = true;
			}
		}
		
		client->setCgiPid(-1);
	}

	if (send_error)
	{
		std::cerr << "Sending 500 Error to Client " << client->getFd() << std::endl;
		client->setStatus(500);
		client->getResponse().generateError();
		client->setState(WRITING);
		set_events(client->getFd(), EPOLLOUT);
	}
}

void	EpollManager::set_events(int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

void	EpollManager::handle_listener_event(int fd)
{
	std::vector<Server*>& server_candidates = listeners[fd];

	Server* primary_server = server_candidates[0];
	while (true)
	{
		int client_fd = primary_server->accept_client();
		if (client_fd == -1)
			break;

		Client* conn = new Client(client_fd, server_candidates);
		if (!add_client(conn)) {
			close(client_fd);
			delete conn;
		}
	}
}

void	EpollManager::handle_client_event(int fd, uint32_t events)
{
	Client* client = clients[fd];
	if (!client)
		return ;

	if ((events & EPOLLHUP) || (events & EPOLLRDHUP) || (events & EPOLLERR))
	{
		std::cerr << "Epoll Error on fd=" << fd << std::endl;
		remove_client(fd);
		return;
	}
	
	client->updateTime();

	switch (client->getState())
	{
		case READING:
			if (events & EPOLLIN)
			{
				client->onReadable();
				if (client->getRequest().getState() >= STATE_HEADERS_DONE)
					client->setState(PROCESSING);
				else
					break ;
			}
			else break;
		case PROCESSING:
		{
			std::cout << "Processing Request..." << std::endl;
			client->process(*this);

			if (client->getState() == PROCESSING)
				client->setState(READING);
			break ;
		}
		case WRITING:
			if (events & EPOLLOUT)
			{
				client->onWritable();
				if (client->getState() == READING)
					set_events(fd, EPOLLIN);
				else if (client->getState() == CLOSING)
					remove_client(fd);
			}
			break;
		case HANDLING_CGI:
		{
			if (events & EPOLLIN)
			{
				char buffer[1];
				ssize_t bytes = recv(fd, buffer, 1, MSG_PEEK);

				if (bytes == 0) 
				{
					std::cerr << "Client disconnected during CGI processing (EOF)" << std::endl;
					remove_client(fd);
					return;
				}
			}
			return ;
		}
		case CLOSING:
		{
			remove_client(fd);
			return ;
		}
	}

	if (client->isClosed())
		remove_client(fd);
}

void	EpollManager::run_loop()
{
	struct epoll_event event_list[MAX_EVENTS];

	while (g_running)
	{
		int n = epoll_wait(epfd, event_list, MAX_EVENTS, timeout_ms);

		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
			break;
		}

		for (int i = 0; i < n; i++)
		{
			int fd = event_list[i].data.fd;
			uint32_t events = event_list[i].events;

			if (listeners.count(fd))
				handle_listener_event(fd);
			else if (clients.count(fd))
				handle_client_event(fd, events);
			else
				handle_cgi_event(fd, events);
		}

		check_client_timeouts();
	}
}

void EpollManager::check_client_timeouts()
{
	time_t current_time = time(NULL);
	const double TIMEOUT_LIMIT = 60.0; 

	std::map<int, Client*>::iterator it = clients.begin();
	
	while (it != clients.end())
	{
		int fd = it->first;
		Client* client = it->second;

		if (difftime(current_time, client->getLastActivity()) > TIMEOUT_LIMIT)
		{
			std::cout << "Timeout: Client " << fd << " idle for " << TIMEOUT_LIMIT << "s. Closing." << std::endl;

			std::map<int, Client*>::iterator next = it;
			++next;

			if (client->getCgiPid() != -1)
			{
				remove_cgi_fds(client, false);

				client->setStatus(504);
				client->getResponse().generateError();
				client->setState(WRITING);
				set_events(fd, EPOLLOUT);
			}
			else
				remove_client(fd);

			it = next;
		}
		else
			++it;
	}
}

/* Constructor */
EpollManager::EpollManager(std::vector<Server>& servers) :
	epfd(-1),
	timeout_ms(1000)
{
	if ((epfd = epoll_create1(0)) == -1) throw std::runtime_error("Failed to create epoll instance");

	if (!_setup_servers(servers)) throw std::runtime_error("No servers could be started!");
}
EpollManager::~EpollManager()
{
	for (std::map<int, Client*>::iterator it = clients.begin();
			it != clients.end(); ++it)
	{
		std::cout << "Cleaning up client fd=" << it->first << std::endl;
		delete it->second;
	}

	close(epfd);

	std::cout << "EpollManager destroyed, all resources cleaned up." << std::endl;
}

bool	EpollManager::_setup_servers(std::vector<Server>& servers)
{
	int success_count = 0;
	std::map<std::string, int> active_ports;
	for (size_t i = 0; i < servers.size(); ++i)
	{
		std::string port = servers[i].getPortStr();
		if (active_ports.find(port) != active_ports.end()) {
			int shared_fd = active_ports[port];
			servers[i].set_fd(shared_fd);
			std::cout << "Virtual Host: Server " << servers[i].getInterfaceStr()
					  << " reusing port " << port << " (FD " << shared_fd << ")" << std::endl;
		}
		else {
			if (!servers[i].init()) {
				std::cerr << "Error: Could not start server on port " << port << std::endl;
				continue;
			}
			active_ports[port] = servers[i].get_fd();
		}
		if (add_listener(&servers[i])) success_count++;
	}
	if (success_count == 0) return false;
	return true;
}
