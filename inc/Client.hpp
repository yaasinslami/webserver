#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <ctime>

#include "Request.hpp"
#include "Response.hpp"
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
		/* states */
		int             _fd;
		ClientState     _state;
		bool            _closed;
		short			_status_code;
		/* http */
		Request		_request;
		Response	_response;
		/* Configs */
		const std::vector<Server*>*	_server_candidates;
		const Server*				_server;
		const Directives*			_config;
		/* CGI */
		pid_t				_cgi_pid;
		int					_cgi_pipe_read_fd;
		int					_cgi_pipe_write_fd;
		std::string			_cgi_response_buf;
		size_t				_cgi_bytes_written;
		/* timeout */
		time_t				_last_activity_time;

		// remove this
		std::string _write_buf;

	public:
		Client(int, const std::vector<Server*>&);
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
		/* Context => Server and Directives */
		void				_setBestConfig();
		bool				hasConfig() const;
		
		Request&			getRequest();
		const Directives*	getConfig() const;
		const Server*				getServer() const;
		
		const short&				getStatus() const;
		void				setStatus(int status);

		void				setState(ClientState state);

		void				setCgiPid(pid_t pid);
		pid_t				getCgiPid() const;

		void				setCgiOutPipe(int fd);
		int					getCgiOutPipe() const;
		
		void				setCgiInPipe(int fd);
		int					getCgiInPipe() const;

		std::string&		get_cgi_response_buf();
		size_t&				get_cgi_bytes_written();

		void				parseCgiResponse();

		void				closeCgiPipes();

		bool				isRequestReady();
		void				updateTime();
		time_t				getLastActivity() const;
};