#pragma once

#include <string>
#include <iostream>
#include <fstream>

#include "Request.hpp"
#include "Server.hpp"
#include "Directives.hpp"

#define BUFFER_SIZE 65536

enum ClientState
{
	READING,
	PROCESSING,
	WRITING,
	HANDLING_CGI,
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

		Server*				_server;
		const Directives*	_config;
		Request				_request;
		int					_status_code;

		// CGI
		pid_t				_cgi_pid;
		int					_cgi_pipe_read_fd;
		int					_cgi_pipe_write_fd;
		
		bool				_is_cgi;
		std::string			_cgi_response_buf;
		size_t				_cgi_bytes_written;

	public:
		Client(int fd, Server* owner_server);
		~Client();


		int					getFd() const;
		bool				isClosed() const;
		void				onReadable();
		void				onWritable();

		ClientState			getState() const;
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

		void				setState(ClientState state);

		void				setCgiPid(pid_t pid);
		pid_t				getCgiPid() const;

		void				setCgiOutPipe(int fd);
		int					getCgiOutPipe() const;
		
		void				setCgiInPipe(int fd);
		int					getCgiInPipe() const;

		bool				isCgi() const;
		void				setIsCgi(bool state);
		std::string&		get_cgi_response_buf();
		size_t&				get_cgi_bytes_written();

		void				closeCgiPipes();

		bool				isRequestReady();

		// i will remove this
		const std::string&	get_read_buf() const;
		const std::string&	get_write_buf() const;
		void				set_write_buf(const std::string &s);

		std::string		_leftover_buffer;
};