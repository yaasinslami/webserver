#include "../../inc/Http.hpp"
#include <vector>

Http::Http() :
	_directives()
{}

Http::~Http() {}

Http::Http(const Http& other)
{
	*this = other;
}

Http&	Http::operator=(const Http& other)
{
	if (this != &other)
	{
		setDirective(other.getDirectives());
		setServers(other.getServers());
	}
	return *this;
}

Directives&	Http::getDirectives()
{
	return this->_directives;
}

const Directives&	Http::getDirectives() const
{
	return this->_directives;
}

const std::vector<Server>&	Http::getServers() const
{
	return this->_servers;
}

void	Http::setDirective(const Directives& directves)
{
	this->_directives = directves;
}

void	Http::setServers(const std::vector<Server>& servers)
{
	this->_servers = servers;
}

void	Http::addServer(const Server& server)
{
	this->_servers.push_back(server);
}

void	Http::logs() const
{
	std::cout << "----------------------------------\n";

	getDirectives().logs();

	for (std::vector<Server>::const_iterator it = _servers.begin(); it != _servers.end(); std::advance(it, 1)) {
		it->logs();
	}
}

void	Http::addto_poll(int fd, short events)
{
	struct pollfd p;

	p.fd = fd;
	p.events = events;
	p.revents = 0;

	_poll_fds.push_back(p);
}

void	Http::setup_servers( void )
{
	for (size_t i = 0; i < _servers.size(); ++i)
	{
		_servers[i].init_server();

		unsigned short	port = _servers[i].get_sockaddr().sin_port;
		_servers_by_port[port].push_back(&_servers[i]);
	}

	for (std::map<unsigned short, std::vector<Server*>>::iterator it = _servers_by_port.begin(); it != _servers_by_port.end(); ++it)
	{
		std::vector<Server*> &servers = it->second;

		Server *leader_server = servers[0];

		int fd;
		if (fd = socket(AF_INET, SOCK_STREAM, 0) == -1)
			throw std::runtime_error("Error: failed to  create a socket");
		
		int	opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (bind(fd, (struct sockaddr *)&leader_server->get_sockaddr(), sizeof(leader_server->get_sockaddr())) == -1) {
			throw std::runtime_error("Error: failed to bind a socket");
		}

		if (listen(fd, 128)) {
			throw std::runtime_error("Error: socket failed to listen");
		}

		for (size_t i = 0; i < servers.size(); ++i) {
			servers[i]->set_fd(fd);
		}

		addto_poll(fd, POLLIN);

		std::cout << "Listening on port " << ntohs(it->first) << " (FD: " << fd << ")" << std::endl;
	}
}

void	Http::run( void )
{
	std::cout << "Server is running..." << std::endl;

	while (true)
	{
		int status = poll(_poll_fds.data(), _poll_fds.size(), -1);

		if (status < 0) {
			throw std::runtime_error("Error: poll failed");
		}

		// if (status == 0) continue;

		for (size_t i = 0; i < _poll_fds.size(); ++i) {
			if (_poll_fds[i].revents & POLLIN) {
				int	curr_fd = _poll_fds[i].fd;

				// DECISION: Is this a "Listener" or a "Client"?
			}
		}
	}
}