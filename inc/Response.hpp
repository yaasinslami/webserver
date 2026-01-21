#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "Client.hpp"

class	Response
{
	private:
		void			_processMethod(Client& client);
		void			_translatePath(const std::string& path, const Location* loc);
		void			_handleGet(const Client&);
		void			_serveFile();
	public:
		Response(Client &client);
		~Response();

	private:
		std::string	_resource;
		int 		_code;
};

# endif