#pragma once

#include "WebServer.hpp"

class	LocationConfig
{
	public:
		LocationConfig();
		~LocationConfig();

		/* Getters */
		std::string					getPath() const;
		std::string					getRoot() const;
		std::string					getAlias() const;
		std::string					getIndex() const;
		std::map<int, std::string>	getErrorPages() const;
		std::vector<std::string>	getMethods() const;
		bool						getAutoIndex() const;
		unsigned long				getMaxBodySize() const;
		
		/* Setters */
		void						setPath(const std::string& path);
		void						setRoot(const std::string &root);
		void						setAlias(const std::string& alias);
		void						setIndex(const std::string& index);
		void						setErrorPages(const std::map<int, std::string>& error_pages);
		void						setMethods(const std::vector<std::string>& allowed_methods);
		void						addMethod(const std::string& method);
		void						setAutoIndex(const bool& autoindex);
		void						setClientMaxBody(const unsigned long& client_max_body_size);

		private:
		std::string					_path;
		std::string					_root;
		std::string					_alias;
		std::string					_index;
		std::map<int, std::string>	_error_pages;
		std::vector<std::string>	_allowed_methods;
		bool						_autoindex;
		unsigned long				_client_max_body_size;
};
