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
		int				_fd;
		ClientState		_state;
		bool			_closed;
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
		std::string			_cgi_response_buf;
		size_t				_cgi_bytes_written;
		/* timeout */
		time_t				_last_activity_time;

		// i will remove this
		std::string _write_buf;

	public:
		/* Constructors */
		Client(int, const std::vector<Server*>&);
		~Client();

		/* Workers */
		void				onReadable();
		void				onWritable();
		void				process();

		/* Getters */
		int					getFd() const;
		bool				isClosed() const;
		Request&			getRequest();
		const Directives*	getConfig() const;
		const Server*		getServer() const;
		ClientState			getState() const;
		const short&		getStatus() const;
		void				setStatus(int status);
		pid_t				getCgiPid() const;
		int					getCgiOutPipe() const;
		std::string&		get_cgi_response_buf();
		size_t&				get_cgi_bytes_written();
		
		/* Setters */	
		void				setState(ClientState state);
		void				setCgiPid(pid_t pid);
		void				setCgiOutPipe(int fd);
		/* State Markers */
		void				markProcessing();
		void				markWriting();
		void				scheduleClose();
		/* Context => Server and Directives */
		void				_setBestConfig();
		bool				hasConfig() const;

		void				parseCgiResponse();

		void				closeCgiPipes();

		bool				isRequestReady();
		void				updateTime();
		time_t				getLastActivity() const;

		// i will remove this
		const std::string&	get_write_buf() const;
		void				set_write_buf(const std::string &s);
	private:
		bool				_validateRequest();
};