#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include "Request.hpp"

class	Client
{
	public:
		Client(int fd);
		Client();
		~Client();

		bool	isRequestReady() const;
		Request &getRequest();

	private:
		int			_fd;
		std::string	_ip_adress;
		std::string	buffer;
		Request		_request;
};

#endif