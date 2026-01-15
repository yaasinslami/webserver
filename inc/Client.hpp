#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include "Request.hpp"

class	Client
{
	public:
		Client();
		Client(int fd, unsigned short server_port);
		Client(const Client& other);
		~Client();

		Client& operator=(const Client& other);
		std::string& getRequestBuffer();

		bool	isRequestReady() const;
		Request &getRequest();
		const Request& getRequest() const;

		unsigned short	getServerPort() const;

		void	show() const;

	private:
		int				_fd;
		unsigned short _server_port;
		Request		_request;
		std::string	_request_buffer;
};

#endif