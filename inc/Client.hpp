#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include "Request.hpp"
#include "Server.hpp"

class	Client
{
	private:
		// Server *_matched_server; // Pointer to the config shared object
		// const Location *_matched_location;

	public:
		Client();
		Client(int fd, unsigned short server_port);
		Client(const Client& other);
		~Client();

		Client& operator=(const Client& other);

		// void	setConfig(Server* s, const Location* l);

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