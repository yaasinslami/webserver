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

	for (std::map<unsigned short, std::vector<Server*> >::iterator it = _servers_by_port.begin(); it != _servers_by_port.end(); ++it)
	{
		std::vector<Server*> &servers = it->second;

		Server *leader_server = servers[0];

		int fd;
		if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			throw std::runtime_error("Error: failed to  create a socket");
		
		int	opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (bind(fd, (struct sockaddr *)&leader_server->get_sockaddr(), sizeof(leader_server->get_sockaddr())) == -1) {
			throw std::runtime_error("Error: failed to bind a socket");
		}

		if (listen(fd, 128) == -1) {
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
				if (isListeningSocket(curr_fd)) {
					handle_new_connection(curr_fd);
				}
				else {
					handle_client_request(curr_fd);
				}
			}
		}
	}
}

bool	Http::isListeningSocket(int fd)
{
	for (size_t i = 0; i < _servers.size(); ++i) {
		if (_servers[i].get_fd() == fd) {
			return true;
		}
	}
	return false;
}

void	Http::handle_new_connection(int listener_fd)
{
	struct sockaddr_in	client_addr;
	socklen_t	addr_len = sizeof(client_addr);

	int	client_fd = accept(listener_fd, (struct sockaddr *)&client_addr, &addr_len);

	if (client_fd < 0) {
		std::cerr << "Error accepting connection" << std::endl;
		return;
	}

	fcntl(client_fd, F_SETFL, O_NONBLOCK);

	addto_poll(client_fd, POLLIN);

	std::cout << "New connection accepted! FD: " << client_fd << std::endl;
}

void Http::handle_client_request(int client_fd)
{
	char buffer[1024];

	// Clear buffer to be safe (optional but good for debugging)
	memset(buffer, 0, sizeof(buffer));

	int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

	// FIX: Check for <= 0 (0 means closed connection, -1 means error)
	if (bytes_read <= 0)
	{
		std::cout << "Client disconnected or Error." << std::endl;
		close(client_fd);
		remove_from_poll(client_fd);
		return;
	}

	// Null-terminate the string so cout doesn't crash
	buffer[bytes_read] = '\0';

	// FIX: actually print the 'buffer'
	std::cout << "\n------------ RAW REQUEST START ------------" << std::endl;
	std::cout << buffer << std::endl;
	std::cout << "------------- RAW REQUEST END -------------" << std::endl;

    // Send the data

	close(client_fd);
	remove_from_poll(client_fd);
}

void	Http::remove_from_poll(int client_fd)
{
	for (std::vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it)
	{
		if (it->fd == client_fd) {
			_poll_fds.erase(it);
			return ;
		}
	}
}
