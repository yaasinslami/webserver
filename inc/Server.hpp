#pragma once

#include <string>
#include <map>
#include <vector>

class	Location;

class	Server
{
	public:
		Server();
		~Server();

		/* Getters */
		std::string					getInterface() const;
		int							getPort() const;
		std::string					getServerName() const;
		std::string					getRoot() const;
		std::string					getIndex() const;
		unsigned long				getMaxBodySize() const;
		std::map<int, std::string>	getErrorPages() const;
		std::vector<Location>		getLocation() const;

		/* Setters */
		void						setInterface(const std::string& interface);
		void						setPort(const int& port);
		void						setServerName(const std::string& server_name);
		void						setRoot(const std::string& root);
		void						setIndex(const std::string& index);
		void						setMaxBodySize(const unsigned long& client_max_body_size);
		void						setErrorPages(const std::map<int, std::string>& error_pages);
		void						setLocation(const std::vector<Location>& locations);

	private:
		std::string					_interface;
		int							_port;
		std::string					_server_name;
		std::string					_root;
		std::string					_index;
		unsigned long				_client_max_body_size;
		std::map<int, std::string>	_error_pages;
		std::vector<Location>		_locations;
};