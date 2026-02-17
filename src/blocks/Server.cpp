#include "Server.hpp"
#include <iostream>

Server::Server(const Directives& g_directives) :
	_directives(g_directives),
	_interface("localhost"),
	_port("4242")
{
	_server_table[INTERFACE];
	_server_table[PORT];
	_server_table[S_NAME];
	_server_table[LOCATION];	
}

Server::~Server() {}

Server& Server::operator=(const Server& other)
{
	if (this != &other) {
		setDirectives(other.getDirectives());
		setInterface(other.getInterfaceStr());
		setPort(other.getPortStr());
		setServerName(other.getServerNames());
		setLocations(other.getLocation());
	}
	return *this;
}

const Directives&	Server::getDirectives() const
{
	return this->_directives;
}

Directives&	Server::getDirectives()
{
	return this->_directives;
}

const std::string&	Server::getInterfaceStr() const
{
	return this->_interface;
}

const std::string&	Server::getPortStr() const
{
	return this->_port;
}

const std::vector<std::string>&	Server::getServerNames() const
{
	return this->_server_name;
}

const std::vector<Location>&		Server::getLocation() const
{
	return this->_locations;
}

int	Server::get_fd( void )
{
	return this->_listen_fd;
}

void Server::setInterface(const std::string &interface)
{
	_server_table[INTERFACE]++;
	this->_interface = interface;
}

void Server::setPort(const std::string &port)
{
    int p = std::atoi(port.c_str());

    if (p < 1 || p > 65535)
        throw std::runtime_error("Port out of range (1-65535): " + port);

    this->_port = port;
}

void Server::setServerName(const std::vector<std::string> &server_name)
{
	_server_name = server_name;
}

void	Server::addServerName(const std::string& server_name)
{
	std::string s_name(server_name);

	++_server_table[S_NAME];
	if (server_name == "localhost")
		s_name = "127.0.0.1";
	this->_server_name.push_back(s_name);
}

void Server::setLocations(const std::vector<Location> &locations)
{
	this->_locations = locations;
}

void	Server::addLocation(const Location& location)
{
	_server_table[LOCATION]++;
	this->_locations.push_back(location);
}

void	Server::setDirectives(const Directives& directves)
{
	this->_directives = directves;
}

void	Server::validate() const
{
	if (_server_table.at(INTERFACE) == 0 && _server_table.at(PORT) == 0)
		throw std::runtime_error("Missing 'listen' directve in server block.");

	if (_server_table.at(INTERFACE) > 1 || _server_table.at(PORT) > 1)
		throw std::runtime_error("Duplicate 'Interface|port' in server block.");

	if (_server_table.at(S_NAME) < 1) {
		std::cout << _server_table.at(S_NAME) << std::endl;
		throw std::runtime_error("Missing 'server_name'directve.");
	}

	if (_server_table.at(LOCATION) < 1)
		throw std::runtime_error("Missing a 'location' block.");
}

void	Server::set_fd(int fd)
{
	this->_listen_fd = fd;
}

void	Server::logs() const
{

	std::cout << "------------- Server --------------\n";
	getDirectives().logs();
	std::cout << "interface: \"" << getInterfaceStr() << "\"\n";
	std::cout << "port: \"" << getPortStr() << "\"\n";


	for (std::vector<Location>::const_iterator it = _locations.begin(); it != _locations.end(); std::advance(it, 1)) {
		it->logs();
	}
	std::cout << std::endl;
}

bool Server::init(void)
{
    struct addrinfo hints;
    struct addrinfo *res;
    int fd = -1;
    int opt = 1;

    Utils::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(_interface.c_str(), _port.c_str(), &hints, &res);
    if (status != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        return false;
    }

    for (struct addrinfo *p = res; p; p = p->ai_next)
    {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1)
        {
            std::cerr << "socket(): " << strerror(errno) << std::endl;
            continue;
        }

        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            std::cerr << "setsockopt(SO_REUSEADDR): " << strerror(errno) << std::endl;
            close(fd);
            fd = -1;
            continue;
        }

        if (bind(fd, p->ai_addr, p->ai_addrlen) == 0)
        {
            std::cout << "Socket " << fd << " bound to " << _interface << ":" << _port << std::endl;
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);

    if (fd == -1)
    {
        std::cerr << "Failed to bind any address on port " << _port << std::endl;
        return false;
    }

    if (!Utils::set_nonblocking(fd))
    {
        std::cerr << "Failed to set socket to non-blocking" << std::endl;
        close(fd);
        return false;
    }

    if (listen(fd, BACKLOG) == -1)
    {
        std::cerr << "listen(): " << strerror(errno) << std::endl;
        close(fd);
        return false;
    }

    _listen_fd = fd;
    return true;
}

int Server::accept_client(void)
{
    struct sockaddr_storage clientAddr;
    socklen_t               addrSize;
    int                     clientFd;
    
    addrSize = sizeof(clientAddr);

    clientFd = accept(_listen_fd, (struct sockaddr*)&clientAddr, &addrSize);
    if (clientFd == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return (-1);
        std::cerr << "accept(): " << strerror(errno) << std::endl;
        return (-1);
    }

    if (!Utils::set_nonblocking(clientFd))
    {
        std::cerr << "Failed to set client socket to non-blocking" << std::endl;
        close(clientFd);
        return (-1);
    }

    std::cout << "New client accepted: fd=" << clientFd << std::endl;
    return (clientFd);
}
