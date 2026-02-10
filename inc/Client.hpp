#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <string>
#include "Request.hpp"
#include "Server.hpp"
#include "Directives.hpp"

class	Client
{
	private:
		int				_fd;
		unsigned short	_server_port;
		bool			_should_close;
		int				_status_code;

		// For responding
		std::ifstream	_file_stream;
		size_t			_bytes_sent;//Track progress
		// size of the file (for get or post)

	public:
		Client();
		Client(int fd, unsigned short server_port);
		Client(const Client& other);
		~Client();

		Client& operator=(const Client& other);

		void	setContext(Server *s, const Location *l);

		int				getFd();
		bool			shouldClose() const;
		bool			shouldKeepAlive() const;
		bool			isRequestReady();
		bool			hasContext() const;
		Request 		&getRequest();
		const Request& 	getRequest() const;
		const Directives* getConfig() const;
		const Server* 	getServer() const;
		unsigned short	getServerPort() const;

		void	setStatus(int);
		int&		getStatus();


		void	show() const;
		void	setClose(bool);

		std::string		_leftover_buffer;

	private:
		Request				_request;
		Server				*_server;
		const Directives	*_config;
};

#endif