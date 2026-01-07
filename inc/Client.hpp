#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include "Request.hpp"

class	Client
{
	public:
		Client();
		Client(int fd);
		Client(const Client& other);
		~Client();

		Client& operator=(const Client& other);

		bool	isRequestReady() const;
		Request &getRequest();

		void	show() const;

	private:
		int			_fd;
		Request		_request;
};

#endif