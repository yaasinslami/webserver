#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "Client.hpp"

class	Response
{
	private:
		void			_processMethod();
		void			_translatePath();
		void			_handleGet();
		// void			_handlePost();
		void			_serveFile();// GET
		void			_buildErrorPage();
		// void			_handleMultiPart();// POST
		// void			_saveFile(std::string&, std::string&);
		
		static std::string		_getMimeType(std::string& path);
		static std::string		_toString(int);
		static std::string		_getStatusMsg(int);
		// static std::string		_extractFilename(std::string&);

	public:
		Response(Client &client);
		~Response();

		void	generateResponse();
		const std::string& getResponseContent() const;

	private:
		Client&		_client;
		std::string	_resource_path;
		int			&_status_code;

		std::map<std::string, std::string>	_headers;
};

# endif