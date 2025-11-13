#pragma once

#include "WebServer.hpp"
#include "Server.hpp"

class	Http
{
	public:
		Http();
		~Http();

		/* Getters */
		std::string					getRoot() const;
		std::string					getIndex() const;
		unsigned long				getMaxBodySize() const;
		std::map<int, std::string>	getErrorPages() const;
		std::vector<Server>			getServers() const;

		/* Setters */
		void	setRoot(const std::string& root);
		void	setIndex(const std::string& index);
		void	setMaxBodySize(const unsigned long& client_max_body_size);
		void	setErrorPages(const std::map<int, std::string>& error_pages);
		void	setServers(const std::vector<Server>& servers);
		void	addServer(const Server& server);
	
	private:
		std::string					_root;
		std::string					_index;
		unsigned long				_client_max_body_size;
		std::map<int, std::string>	_error_pages;
		std::vector<Server>			_servers;
};