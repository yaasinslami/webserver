#include "../../inc/Server.hpp"

Server::Server(const Directives& g_directives) :
	_directives(g_directives),
	_interface("localhost"),
	_port("80"),
	_server_name("")
{
	_server_table[INTERFACE];
	_server_table[PORT];
	_server_table[S_NAME];
	_server_table[LOCATION];	
}

Server::~Server() {}

Server& Server::operator=(const Server& other)
{
	if (this != &other)
	{
		setDirectives(other.getDirectives());
		setInterface(other.getInterfaceStr());
		setPort(other.getPortStr());
		setServerName(other.getServerName());
		setLocations(other.getLocation());
	}
	return *this;
}

void	Server::init_server( void )
{
	struct addrinfo hints;
	struct addrinfo *result;
	int status;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(getInterfaceStr().c_str(), getPortStr().c_str(), &hints, &result);

	if (status != 0)
	{
		// The lookup failed (e.g., invalid hostname, port number out of range)
		std::cerr << "Error in getaddrinfo for [" << _interface << ":" << _port
				  << "]: " << gai_strerror(status) << std::endl;

		throw std::runtime_error("");
	}

	memset(&get_sockaddr(), 0, sizeof(get_sockaddr()));
	set_sockaddr(*(struct sockaddr_in *)result->ai_addr);
	get_sockaddr().sin_family = AF_INET;

	freeaddrinfo(result);
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

const std::string&	Server::getServerName() const
{
	return this->_server_name;
}

const std::vector<Location>&		Server::getLocation() const
{
	return this->_locations;
}

struct sockaddr_in&	Server::get_sockaddr( void )
{
	return this->_server_sockaddr;
}

void Server::setInterface(const std::string &interface)
{
	_server_table[INTERFACE]++;
	this->_interface = inet_addr(interface.c_str());
}

void Server::setPort(const std::string &port)
{
	_server_table[PORT]++;
	// if (port < 1 || port > 65636)
	// 	throw std::runtime_error("Wrong syntax: port");
	this->_port = port;
}

void Server::setServerName(const std::string &server_name)
{
	_server_table[S_NAME]++;
	this->_server_name = server_name;
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

	if (_server_table.at(S_NAME) != 1)
		throw std::runtime_error("Missing 'server_name'directve.");

	if (_server_table.at(LOCATION) < 1)
		throw std::runtime_error("Missing a 'location' block.");
}

void	Server::set_sockaddr(const struct sockaddr_in& sockaddr)
{
	this->_server_sockaddr = sockaddr;
}

void	Server::set_fd(int fd)
{
	this->_listen_fd = fd;
}

void	Server::logs() const
{
	std::cout << "Server [" << getServerName() << "]\n";

	getDirectives().logs();
	std::cout << "interface: \"" << getInterfaceStr() << "\"\n";
	std::cout << "port: \"" << getPortStr() << "\"\n";


	for (std::vector<Location>::const_iterator it = _locations.begin(); it != _locations.end(); std::advance(it, 1)) {
		it->logs();
	}
	std::cout << std::endl;
}
