#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "Client.hpp"

class	Response
{
	private:
		void			_processMethod(Client& client);
		std::string		_translatePath(const std::string& path, const Location* loc);
	public:
		Response(Client &client);
		~Response();

	private:
		int _code;
};

# endif