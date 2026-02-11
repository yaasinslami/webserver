#include "EpollManager.hpp"

EpollManager::EpollManager(int timeout)
	: timeout_ms(timeout), running(false)
{
	epfd = epoll_create1(0);

	cout << "epoll instance : " << epfd << endl;

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
	ev.events = EPOLLIN | EPOLLET;
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

	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

	delete clients[fd];
	clients.erase(fd);

	return (true);
}

void	EpollManager::set_events(int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events | EPOLLET;
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

void	EpollManager::handle_client_event(int fd, uint32_t events)
{
	Client* conn = clients[fd];
	if (!conn)
		return ;

	if (events & (EPOLLERR | EPOLLHUP))
	{
		conn->scheduleClose();
		return ;
	}

	if (events & EPOLLIN)
	{
		cout << "EPOLLIN Event accured" << endl;
		conn->onReadable();
	}

	if (conn->isClosed())
	{
		remove_client(fd);
		return ;
	}

	if (conn->isRequestReady())
	{
		conn->markProcessing();

		std::string response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: 12\r\n\r\n"
				"Hello World\n";

		conn->set_write_buf(response);
		conn->markWriting();
		set_events(fd, EPOLLOUT);
	}

	if (events & EPOLLOUT)
	{
		conn->onWritable();
		if (!conn->get_write_buf().empty())
			return;

		set_events(fd, EPOLLIN);
	}

	if (conn->isClosed())
		remove_client(fd);
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

			cout << "Process fd : " << fd << endl;

			if (listeners.count(fd))
			{
				cout << "call handle_listener_event" << endl;
				handle_listener_event(fd);
			}
			else
			{
				cout << "call handle_client_event" << endl;
				handle_client_event(fd, events);
			}

			cout << "-------------------------" << endl;
		}
	}
}

void EpollManager::stop()
{
	running = false;
}
