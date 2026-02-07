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

	public:
		Client();
		Client(int fd, unsigned short server_port);
		Client(const Client& other);
		~Client();

		Client& operator=(const Client& other);

		void	setContext(Server *s, const Location *l);

		int			getFd();
		bool		shouldClose() const;
		std::string& getRequestBuffer();
		const char*	getResponseBuffer() const;
		size_t	getResponseSize() const;
		bool	isRequestReady();
		Request &getRequest();
		const Request& getRequest() const;

		unsigned short	getServerPort() const;

		void	show() const;

		const Location* getLocation() const;
		const Server* getServer() const;

		void	setClose(bool);
		bool	shouldKeepAlive() const;
		void	setResponseBuffer(const std::string&);
		void	updateResponseBuffer(size_t n);
		bool	hasDataToSend() const;

		std::string		_leftover_buffer;

	private:
		Request			_request;
		// instead of Server and Location separately i will have Directive 
		// Directive config
		const Directives* _config;
};

#endif