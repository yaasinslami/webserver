#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <iostream>
#include <fstream>

#include "Request.hpp"
#include "Response.hpp"
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
		short			_status_code;

	public:
		Client(int, Server*);
		~Client();

		/* Getters */
		int					getFd() const;
		ClientState			getState() const;
		const short&		getStatus() const;
		void				setStatus(int status);
		Request&			getRequest();
		const Directives*	getConfig() const;
		const Server*		getServer() const;
		bool				isClosed() const;
		/* Workers methods */
		void				onReadable();
		void				onWritable();
		void				process();
		/* State methods */
		void				markProcessing();
		void				markWriting();
		void				scheduleClose();
		/* Context => Server and Directives */
		void				_setBestConfig();
		bool				hasConfig() const;

	private:
		Request				_request;
		Response			_response;
		const Server*		_server;// Client should have a vector of servers bind to the same port
		const Directives*	_config;
};

#endif