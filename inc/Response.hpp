#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "Client.hpp"

class	Response
{
	private:
		void			_processMethod();
		void			_translatePath();
		void			_handleGet();
		void			_serveFile();
		void			_buildErrorPage();
		
		static std::string		_getMimeType(std::string& path);
		static std::string		_toString(int);
		static std::string		_getStatusMsg(int);

	public:
		Response(Client &client);
		~Response();

		void	generateResponse();
		const std::string& getResponseContent() const;

	private:
		Client&		_client;
		std::string	_resource;
		std::string	_body;
		std::string	_response_content;
		int 		_code;
		std::map<std::string, std::string>	_headers;
};

# endif