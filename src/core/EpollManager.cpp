#include "EpollManager.hpp"

static string get_cgi_executable(string, Client&);

void	EpollManager::handle_client_event(int fd, uint32_t events)
{
	Client* client = clients[fd];
	if (!client)
		return ;

	if ((events & EPOLLHUP) || (events & EPOLLRDHUP) || (events & EPOLLERR))
	{
		cerr << "Epoll Error on fd=" << fd << endl;
		if (client->getCgiPid() != -1)
			remove_cgi_fds(client, false);

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

				if (client->getState() ==  CLOSING)
					break ;
			}
			else
				break;
		case PROCESSING:
		{
			std::cout << "Processing Request..." << std::endl;
			client->getRequest().log();

			std::string executable_path = get_cgi_executable(client->getRequest().getPath(), *client);

			if (client->getRequest().getIsCgi())
			{
				std::string	script_fs_path = CgiHandler::prepare(client, *this, fd);
				if (script_fs_path.empty())
					break ;
				CgiHandler::execute(client, script_fs_path, executable_path, this);
			}
			else
			{
				std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World\n";
				client->set_write_buf(response);
				client->markWriting();
				set_events(fd, EPOLLOUT);
			}
		}
			break;
		case WRITING:
			if (events & EPOLLOUT)
			{
				client->onWritable();
				if (client->getState() == READING)
					set_events(fd, EPOLLIN);
				else if (client->getState() == CLOSING)
				{
					remove_client(fd);
					return ;
				}
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
					if (client->getCgiPid() != -1)
						remove_cgi_fds(client, false);
					remove_client(fd);
					return;
				}
			}
			return ;
		}
		case CLOSING:
		{
			// Todo
			if (client->getCgiPid() != -1)
				remove_cgi_fds(client, false);
			remove_client(fd);
			return ;
		}
	}

	if (client->isClosed())
		remove_client(fd);
}

void EpollManager::handle_cgi_event(int fd, uint32_t events)
{
	Client* client = _cgi_fd_map[fd];
	if (!client)
		return;

	client->updateTime();
	
	if (events & EPOLLERR)
	{
		std::cerr << "⚠️ CGI Pipe Error on fd=" << fd << std::endl;
		remove_cgi_fds(client, true);
		return;
	}

	if (fd == client->getCgiInPipe() && (events & EPOLLOUT))
	{
		const std::string& body = client->getRequest().getBody();
		size_t& bytes_written = client->get_cgi_bytes_written(); 

		if (bytes_written >= body.length())
		{
			epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
			_cgi_fd_map.erase(fd);
			close(fd);
			client->setCgiInPipe(-1);
			return;
		}

		size_t remaining = body.length() - bytes_written;
		ssize_t sent = write(fd, body.c_str() + bytes_written, remaining);

		if (sent > 0)
			bytes_written += sent;
		else
			return;

		if (bytes_written >= body.length())
		{
			epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
			_cgi_fd_map.erase(fd);
			close(fd);
			client->setCgiInPipe(-1);
		}
	}

	if (fd == client->getCgiOutPipe() && (events & (EPOLLIN | EPOLLHUP)))
	{
		char buf[4096];
		ssize_t bytes = read(fd, buf, sizeof(buf));

		if (bytes > 0)
		{
			client->get_cgi_response_buf().append(buf, bytes);
		}
		else if (bytes == 0)
		{
			remove_cgi_fds(client, false); 

			// I will replace by real response logique
			if (client->getStatus() != 500)
			{
				client->parseCgiResponse();
				client->markWriting();
				set_events(client->getFd(), EPOLLOUT);
			}
			client->get_cgi_response_buf().clear();
		}
		else
			return;
	}
}


EpollManager::EpollManager(int timeout)
	: timeout_ms(timeout), running(false)
{
	epfd = epoll_create1(0);

	if (epfd == -1)
	{
		throw std::runtime_error("Failed to create epoll instance");
	}
}

EpollManager::~EpollManager()
{
	for (std::map<int, Client*>::iterator it = clients.begin();
			it != clients.end(); ++it)
		delete it->second;

	close(epfd);
}

bool EpollManager::add_listener(Server *srv)
{
	if (!srv) return false;

	int fd = srv->get_fd();
	if (fd < 0) return false;

	if (listeners.find(fd) == listeners.end()) {
		// This is the first server for this port. Add it to Epoll. (Defualt)
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = fd;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
			return false;
		}
		std::cout << "[Epoll] Listening on new fd=" << fd << std::endl;
	}
	else {
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
	if (client->getCgiInPipe() != -1 || client->getCgiOutPipe() != -1)
		remove_cgi_fds(client, false);

	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

	delete clients[fd];
	clients.erase(fd);

	return (true);
}

bool	EpollManager::add_cgi_fds(Client *client)
{
	int out_pipe = client->getCgiOutPipe();
	int in_pipe = client->getCgiInPipe();
	
	struct epoll_event ev;

	ev.data.fd = out_pipe;
	ev.events = EPOLLIN;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, out_pipe, &ev) == -1)
		return (false);
	
	_cgi_fd_map[out_pipe] = client;

	// only for POST request
	if (in_pipe != -1)
	{
		ev.data.fd = in_pipe;
		ev.events = EPOLLOUT;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, in_pipe, &ev) == -1)
			return false;
		_cgi_fd_map[in_pipe] = client;
	}

	return true;
}

void EpollManager::remove_cgi_fds(Client *client, bool send_error)
{
	int read_fd = client->getCgiOutPipe();
	int write_fd = client->getCgiInPipe();
	pid_t pid = client->getCgiPid();

	if (read_fd != -1)
	{
		epoll_ctl(epfd, EPOLL_CTL_DEL, read_fd, NULL);
		close(read_fd);
		_cgi_fd_map.erase(read_fd);
	}
	if (write_fd != -1)
	{
		epoll_ctl(epfd, EPOLL_CTL_DEL, write_fd, NULL);
		close(write_fd);
		_cgi_fd_map.erase(write_fd);
	}

	client->setCgiInPipe(-1);
	client->setCgiOutPipe(-1);

	if (pid != -1)
	{
		int status;
		pid_t result = waitpid(pid, &status, WNOHANG);

		if (result == 0) 
		{
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0); 
			send_error = true;
		}
		else if (result == pid)
		{
			if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
				send_error = true;
		}
		
		client->setCgiPid(-1);
	}

	if (send_error)
	{
		std::cerr << "Sending 500 Error to Client " << client->getFd() << std::endl;
		client->setStatus(500);

		std::string body = "<html>\r\n"
						"<head><title>500 Internal Server Error</title></head>\r\n"
						"<body>\r\n"
						"<center><h1>500 Internal Server Error</h1></center>\r\n"
						"<hr><center>webserv</center>\r\n"
						"</body>\r\n"
						"</html>\r\n";

		std::string response = "HTTP/1.1 500 Internal Server Error\r\n"
							"Content-Type: text/html\r\n"
							"Content-Length: " + Utils::toString(body.length()) + "\r\n"
							"Connection: close\r\n\r\n" + 
							body;

		client->set_write_buf(response);

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

static string get_cgi_executable(string path, Client& client)
{
	const Directives* config = client.getConfig();
	const map<string, string>& handlers = config->getCgiHandlers();

	if (handlers.empty())
		return "";
	
	size_t q_pos = path.find('?');
	if (q_pos != string::npos)
		path = path.substr(0, q_pos);

	size_t dot_pos = path.rfind('.');
	if (dot_pos == string::npos)
		return "";

	string extension = path.substr(dot_pos);
	
	map<string, string>::const_iterator it = handlers.find(extension);

	if (it != handlers.end())
	{
		client.getRequest().setIsCgi(true);
		return it->second;
	}

	return "";
}

void	EpollManager::run_loop()
{
	struct epoll_event event_list[MAX_EVENTS];

	running = true;

	while (running)
	{
		int n = epoll_wait(epfd, event_list, MAX_EVENTS, timeout_ms);

		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < n; i++)
		{
			int fd = event_list[i].data.fd;
			uint32_t events = event_list[i].events;

			if (listeners.count(fd))
			{
				cout << "call handle_listener_event" << endl;
				handle_listener_event(fd);
			}
			else if (clients.count(fd))
			{
				cout << "call handle_client_event" << endl;
				handle_client_event(fd, events);
			}
			else
			{
				cout << "call handle_cgi_event" << endl;;
				handle_cgi_event(fd, events);
			}

			cout << "-------------------------" << endl;
		}

		check_client_timeouts();
	}
}

void EpollManager::stop()
{
	running = false;
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
				remove_cgi_fds(client, false);

			remove_client(fd);

			it = next;
		}
		else
			++it;
	}
}

