#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <iostream>
#include <fstream>

#include "Request.hpp"
#include "Server.hpp"

#define BUFFER_SIZE 65536

enum ClientState
{
	READING,
	PROCESSING,
	WRITING,
	CLOSING
};

class Client
{
	private:
		int				_fd;
		ClientState		_state;
		bool			_closed;
		int				_status_code;

	public:
		Client(int, Server*);
		~Client();


		int					getFd() const;
		bool				isClosed() const;
		void				onReadable();
		void				onWritable();

		ClientState			getState() const;
		void				markProcessing();
		void				markWriting();
		void				scheduleClose();

		int&				getStatus();
		void				setStatus(int status);

		void				setContext(Server *s, const Location *l);
		bool				hasContext() const;
		
		Request&			getRequest();
		const Directives*	getConfig() const;
		const Server*		getServer() const;		
	
	private:
		Request				_request;
		// Response			_response;
		const Server*		_server;
		const Directives*	_config;
};

#endif