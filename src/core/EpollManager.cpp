#include "EpollManager.hpp"

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

bool	EpollManager::add_listener(Server* srv)
{
	if (!srv)
		return false;

	int fd = srv->get_fd();
	if (fd < 0 || listeners.count(fd))
		return false;

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		return false;

	listeners[fd] = srv;

	std::cout << "[Epoll] Listening fd=" << fd << std::endl;
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
		client->set_write_buf("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
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
	Server* srv = listeners[fd];
	if (!srv)
		return;

	while (true)
	{
		int client_fd = srv->accept_client();
		if (client_fd == -1)
			break;

		Client* conn = new Client(client_fd, srv);
		if (!add_client(conn))
		{
			close(client_fd);
			delete conn;
		}
	}
}

static string get_cgi_executable(string path, const Directives* config)
{
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
		return it->second;

	return "";
}

void	EpollManager::handle_client_event(int fd, uint32_t events)
{
	Client* client = clients[fd];
	if (!client)
		return ;

	if (events & (EPOLLERR | EPOLLHUP))
	{
		cerr << "⚠️  Epoll Error on fd=" << fd << endl;
		remove_client(fd);
		return ;
	}

	switch (client->getState())
	{
		case READING:
			if (events & EPOLLIN)
			{
				cout << "EPOLLIN Event occurred" << endl;
				client->onReadable();

				if (client->isRequestReady())
					client->markProcessing();
				else
					break;
			}
			else
				break;
		case PROCESSING:
		// TODO: we will replace this by the RESPONSE generation logique.
		{
			std::cout << "Processing Request..." << std::endl;

			// TODO : we will replace this by CGI responce detection logiqie isCGi() 
			const Directives* config = client->getConfig();
			string executable_path = get_cgi_executable(client->getRequest().getPath(), config);

			cout << "Path : " << client->getRequest().getPath() << endl;
			cout << "executable_path : " << executable_path << endl;

			if (!executable_path.empty())
			{
				Request& req = client->getRequest();
				string	path = req.getPath();
				size_t	q_pos = path.find('?');
				if (q_pos != string::npos)
					path = path.substr(0, q_pos);

				string script_fs_path = Utils::normalize_path(config->getRoot() + path);

				cout << "cgi script : " << script_fs_path << endl;

				if (access(script_fs_path.c_str(), F_OK) == -1)
				{
					client->setStatus(400);
					cerr << "Cgi script file Not found : " << script_fs_path << endl;
					return ;
				}
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
				cout << "send response to Client" << endl;

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
			return ;
		case CLOSING:
			remove_client(fd);
			return ;
	}

	if (client->isClosed())
		remove_client(fd);
}

void EpollManager::handle_cgi_event(int fd, uint32_t events)
{
	Client* client = _cgi_fd_map[fd];
	if (!client)
		return;

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
		else if (bytes == 0) // EOF
		{
			remove_cgi_fds(client, false); 

			// I will replace by real response logique
			if (client->getStatus() != 500)
			{
				std::string final_response = "HTTP/1.1 200 OK\r\n" + client->get_cgi_response_buf();
				
				client->set_write_buf(final_response);
				client->markWriting();
				set_events(client->getFd(), EPOLLOUT);
			}
			client->get_cgi_response_buf().clear();
		}
		else
			return;
	}
}

void	EpollManager::run_loop()
{
	const int MAX_EVENTS = 128;
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
	}
}

void EpollManager::stop()
{
	running = false;
}
