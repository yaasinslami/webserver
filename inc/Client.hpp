#pragma once

#include "Request.hpp"
#include "Response.hpp"
#include "Server.hpp"
#include "Directives.hpp"
#include "CgiHandler.hpp"

#define BUFFER_SIZE 65536

class Request;
class Response;
class Server;
class Directives;
class EpollManager;

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
		/* timeout */
		time_t				_last_activity_time;

		// ifstream temp file
		ifstream			_file_stream;

		// CGI
		std::string			_cgi_temp_path;
		std::ofstream		_cgi_out_file;
		size_t				_cgi_header_size;
		std::string			_cgi_header_buf;

	public:
		/* Constructors */
		Client(int, const std::vector<Server*>&);
		~Client();

		/* Workers */
		void				onReadable();
		void				onWritable();
		void				process(EpollManager& poller);

		/* Getters */
		int					getFd() const;
		bool				isClosed() const;
		Request&			getRequest();
		Response&			getResponse();
		const Directives*	getConfig() const;
		const Server*		getServer() const;
		ClientState			getState() const;
		const short&		getStatus() const;
		void				setStatus(int status);
		pid_t				getCgiPid() const;
		int					getCgiOutPipe() const;

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
		void				resetForNextRequest();

		/* CGI */
		void				parseCgiResponse();
		void				openCgiTempFile();
		void				writeToCgiTempFile(const char* buf, ssize_t size);
		void 				closeCgiOutFile();
		bool				isCgiFileOpen() const;
		std::string& 		getCgiHeaderBuf();

		bool				isRequestReady();
		void				updateTime();
		time_t				getLastActivity() const;

	private:
		bool				_validateRequest();
		void				_serveLargeFile();
};