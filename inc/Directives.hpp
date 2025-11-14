#pragma once

#include "WebServer.hpp"

struct	Directives
{
	public:
		Directives();
		~Directives();

		Directives& operator=(const Directives& dir);

		/* Getters */
		const std::string&					getRoot() const;
		const std::string&					getIndex() const;
		const bool&							getAutoIndex() const;
		const std::vector<std::string>&		getAllowedMethods() const;
		const unsigned long&				getMaxBodySize() const;
		const std::map<int, std::string>&	getErrorPages() const;

		/* Setters */
		void	setRoot(const std::string& root);
		void	setIndex(const std::string& index);
		void	setAutoIndex(const bool& autoindex);
		void	setAllowedMethods(const std::vector<std::string>& allowed_methods);
		void	addMethod(const std::string& method);
		void	setMaxBodySize(const unsigned long& client_max_body_size);
		void	setErrorPages(const std::map<int, std::string>& error_pages);

	private:
		std::string					_root;
		std::string					_index;
		bool						_autoindex;
		std::vector<std::string>	_allowed_methods;
		unsigned long				_client_max_body_size;
		std::map<int, std::string>	_error_pages;
};
