#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "Client.hpp"

class	Response
{
	private:
		void	_processMethod(Client& client);
	public:
		Response(Client &client);
		~Response();

	private:
		int _code;
};

# endif