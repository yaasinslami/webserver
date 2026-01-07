#include "../../inc/Http.hpp"
#include <vector>
#include "Request.hpp"

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

		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
			throw std::runtime_error("Error: failed to  create a socket");
		
		int	opt = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		if (bind(fd, (struct sockaddr *)&leader_server->get_sockaddr(), sizeof(leader_server->get_sockaddr())) == -1) {
			throw std::runtime_error("Error: failed to bind a socket");
		}

		if (listen(fd, SOMAXCONN) == -1) {
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

	// event loop
	while (g_running)
	{
		int status = poll(_poll_fds.data(), _poll_fds.size(), -1);

		if (status < 0) {
			throw std::runtime_error("Error: poll failed");
		}

		// if (status == 0) continue;

		// loop over the Interest List
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
	std::cout << "Server shutting down cleanly..." << std::endl;
}

void Http::handle_new_connection(int listener_fd)
{
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	int client_fd = accept(listener_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd < 0) {
		std::cerr << "Error accepting connection" << std::endl;
		return;
	}

	fcntl(client_fd, F_SETFL, O_NONBLOCK);
	addto_poll(client_fd, POLLIN);

	// create a client with that fd and store it
	Client new_client(client_fd);
	_clients[client_fd] = new_client;

	std::cout << "\nNew connection from " << inet_ntoa(client_addr.sin_addr) << ":" << htons(client_addr.sin_port)
			  << " (FD: " << client_fd << ")" << std::endl;

}

void Http::handle_client_request(int client_fd)
{
	Client	&curr_client = _clients[client_fd];

	char	tmp_buffer[4096];
	memset(tmp_buffer, 0, sizeof(tmp_buffer));
	ssize_t	bytes_read;

	// loop over recv until request is done
	while (true)
	{
		std::cout << "recv ...\n";
		bytes_read = recv(client_fd, tmp_buffer, sizeof(tmp_buffer) - 1, 0);

		if (bytes_read < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				std::cout << "-- farmina dakche li khass ytfarma akhoya --\n";
				break;
			}

			std::cerr << "Recv Error: " << strerror(errno) << std::endl;
			close_connection(client_fd);
			return ;
		}
		else if (bytes_read == 0)
		{
			std::cout << "Client " << client_fd << " disconnected cleanly." << std::endl;
			close_connection(client_fd);
			break ;
		}
		// we have data
		try {
			std::cout << "feeding ...\n";
			curr_client.getRequest().feed(tmp_buffer, bytes_read);
		}
		catch(std::exception& e) {
			std::cerr << "Request Error: " << e.what() << std::endl;
			close_connection(client_fd);
			return ;
		}
		
		//check if client request fully readed
		// if (curr_client.isRequestReady()) {
		// 	std::cerr << "---- Salina l farming ----\n";
		// 	break ;
		// }
	}

	// exit the while, we expect we have finished data - now let chose the best config match specific criteria based on that request then do the parse number 2 to check validity 
	// ...

	curr_client.show();

	// now lets generate the respounse based on the request method ....
	if (curr_client.isRequestReady())// the request should be completed here
    {
		// 1. Process and Send Response
		std::cout << "Request Ready! Sending Response..." << std::endl;
		// by now just close connection | no Keep-Alive logic
		close_connection(client_fd);

		// 2. Logic for Keep-Alive
		// if (curr_client.getRequest().shouldKeepAlive()) {
		// 	curr_client.getRequest().resetState();// reset state to STATE_READING_HEADERS
		// }
		// else {
		// 	close_connection(client_fd);
		// }
	}
	else {
		std::cerr << "Misconigured Request" << std::endl;
		close_connection(client_fd);
	}
}

void	Http::close_connection(int client_fd)
{
	remove_from_poll(client_fd);
	close(client_fd);
	_clients.erase(client_fd);
}

void	Http::remove_from_poll(int client_fd)
{
	for (std::vector<struct pollfd>::iterator it = _poll_fds.begin(); it != _poll_fds.end(); ++it)
	{
		if (it->fd == client_fd) {
			close(it->fd);
			_poll_fds.erase(it);
			return ;
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

void	Http::addto_poll(int fd, short events)
{
	struct pollfd p;

	p.fd = fd;
	p.events = events;
	p.revents = 0;

	_poll_fds.push_back(p);
}


Http::Http() :
	_directives()
{}

Http::~Http()
{
	for (size_t i = 0; i < _poll_fds.size(); i++)
	{
		if (_poll_fds[i].fd >= 0) {
			close(_poll_fds[i].fd);
		}
	}
}

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
