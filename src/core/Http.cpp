#include "../../inc/Http.hpp"
#include <vector>
#include "Request.hpp"

void	Http::setup_servers( void )
{
	for (size_t i = 0; i < _servers.size(); ++i)
	{
		_servers[i].init_server();

		unsigned short port = ntohs(_servers[i].get_sockaddr().sin_port);
		_servers_by_port[port].push_back(&_servers[i]);
		std::cout << "port=" << port << std::endl;
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

	// 2. FIND THE LOCAL PORT (Server Port)
	struct sockaddr_in server_addr;
	socklen_t server_len = sizeof(server_addr);
	if (getsockname(client_fd, (struct sockaddr *)&server_addr, &server_len) == -1)
	{
		std::cerr << "getsockname failed" << std::endl;
		close(client_fd);
		return;
	}
	// Convert to host byte order (e.g., 8080)
	unsigned short server_port = ntohs(server_addr.sin_port);

	// create a client with that fd and store it
	Client new_client(client_fd, server_port);
	_clients[client_fd] = new_client;

	std::cout << "\n[" << client_fd << "] -> New connection from " << inet_ntoa(client_addr.sin_addr) 
              << ":" << htons(client_addr.sin_port) 
              << " on Server Port: " << server_port << std::endl;
}

void Http::handle_client_request(int client_fd)
{
	Client	&curr_client = _clients[client_fd];
	bool	client_disconnected = false;

	char	tmp_buffer[4096];
	memset(tmp_buffer, 0, sizeof(tmp_buffer));
	ssize_t	bytes_read = 0;

	// loop over recv until request is done
	while (true)
	{
		std::cout << "recv ...\n";
		bytes_read = recv(client_fd, tmp_buffer, sizeof(tmp_buffer) - 1, 0);

		if (bytes_read < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				std::cout << "errno == EWOULDBLOCK || errno == EAGAIN => break!\n";
				break;
			}

			std::cerr << "Recv Error: " << strerror(errno) << std::endl;
			close_connection(client_fd);
			return ;
		}
		else if (bytes_read == 0)
		{
			std::cout << "Client " << client_fd << " disconnected (EOF)." << std::endl;
			client_disconnected = true;
			break ;
		}
		else {
			curr_client.getRequestBuffer().append(tmp_buffer, bytes_read);
		}
	}

	while (!curr_client.getRequestBuffer().empty()) {
		std::cout << "feeding...\n";
		try {
			// std::cout << "=== what will be parsed now ====\n";
			// std::cout << curr_client.getRequestBuffer() << std::endl;
			size_t	consumed = curr_client.getRequest().feed(curr_client.getRequestBuffer());
			std::cout << "terminating feeding...\n";
			curr_client.getRequestBuffer().erase(0, consumed);
			// now lets generate the respounse based on the request method ....
			if (curr_client.isRequestReady())// the request should be completed here
			{
				curr_client.show();
				// Process and Send Response
				std::cout << "Request Ready! Sending Response..." << std::endl;

				Server* server =  _find_best_match_server(curr_client);
				if (server == NULL) {
					close_connection(client_fd);
					return ;
				}

				const Location*	location = findLocation(curr_client.getRequest().getPath());

				if (!location) {
					// 404 or use a default internal locationation
				}

				// 3. Store in Client (State belongs to Client, not Server)
				curr_client.setConfig(server, location);

				// Response response(curr_client.getRequest(), *server, *loc);

				// Logic for Keep-Alive
				if (curr_client.getRequest().shouldKeepAlive()) {
					curr_client.getRequest().resetState();// reset state to STATE_READING_HEADERS
				}
				else {
					std::cout << "Connection: close requested. Closing.\n";
					close_connection(client_fd);
					return ;
				}
			}
			else {
				std::cout << "Request not ready yet, wait for more data\n";
				break ;
			}
		}
		catch (const std::exception& e) {
			std::cerr << "🚫 Bad Request: " << e.what() << "\n";
			close_connection(client_fd);
			return ;
		}
	}
	// curr_client.show();
	if (client_disconnected) {
		std::cout << "Cleaning up disconnected client.\n";
		close_connection(client_fd);
	}
}

Server* Http::_find_best_match_server(const Client& client)
{
	unsigned short	port = client.getServerPort();
	std::string host_name = client.getRequest().getServerName();

	if (_servers_by_port.find(port) == _servers_by_port.end() || _servers_by_port[port].empty()) {
		std::cerr << "Error: No servers found for port " << port << std::endl;
		return NULL;
	}

	std::vector<Server*>& candidates = _servers_by_port[port];

	for (size_t i = 0; i < candidates.size(); ++i)
	{
		const std::vector<std::string> &names = candidates[i]->getServerNames();

		for (size_t j = 0; j < names.size(); ++j)
		{
			if (host_name == names[j]){
				std::cout << "Server match found: " << names[j] << std::endl;
				return candidates[i];
			}
		}
	}

	// FALLBACK
	std::cout << "No name match. Using default server for port " << port << std::endl;
	return candidates[0];
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
