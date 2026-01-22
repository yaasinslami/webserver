#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include "Request.hpp"
#include "Server.hpp"

class	Client
{
	private:
		int				_fd;
		unsigned short _server_port;
		std::string	_request_buffer;

	public:
		Client();
		Client(int fd, unsigned short server_port);
		Client(const Client& other);
		~Client();

		Client& operator=(const Client& other);

		void	setContext(Server *s, const Location *l);

		int			getFd();
		std::string& getRequestBuffer();
		bool	isRequestReady() const;
		Request &getRequest();
		const Request& getRequest() const;

		unsigned short	getServerPort() const;

		void	show() const;

		const Location* getLocation() const;
		const Server* getServer() const;

	private:
		Request		_request;
		Server			*_server;
		const Location	*_location;
};

#endif