#pragma once

#include <string>
#include <iostream>
#include <fstream>

#include "Request.hpp"
#include "Server.hpp"
#include "Directives.hpp"

#define BUFFER_SIZE 4096

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
		int             _fd;
		ClientState     _state;
		std::string     _read_buf;
		std::string     _write_buf;
		bool            _closed;

		Server*             _server;
		const Directives*   _config;
		Request             _request;
		int                 _status_code;

	public:
		Client(int fd, Server* owner_server);
		~Client();


		int					getFd() const;
		bool				isClosed() const;
		void				onReadable();
		void				onWritable();

		void				markProcessing();
		void				markWriting();
		void				setWriteBuf(const std::string &s);
		void				scheduleClose();

		void				setContext(Server *s, const Location *l);
		bool				hasContext() const;
		
		Request&			getRequest();
		const Directives*	getConfig() const;
		Server*				getServer() const;
		
		int&				getStatus();
		void				setStatus(int status);

		bool				isRequestReady();

		// i will remove this
		const std::string&	get_read_buf() const;
		const std::string&	get_write_buf() const;
		void				set_write_buf(const std::string &s);

		std::string		_leftover_buffer;
};