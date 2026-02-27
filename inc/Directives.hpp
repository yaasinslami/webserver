#pragma once

#include <string>
#include <vector>
#include <map>

#define ABSOLUTE_MAX_BODY_SIZE 104857600 // 100 MB

class	ConfigParser;

enum e_directive_type
{
	DIR_SERVER,
	DIR_LOCATION,
	DIR_ROOT,
	DIR_LISTEN,
	DIR_INDEX,
	DIR_AUTO_INDEX,
	DIR_ALIAS,
	DIR_MAX_BODY_SIZE,
	DIR_ALLOWED_METHODS,
	DIR_SERVER_NAME,
	DIR_ERROR_PAGES,
	DIR_UPLOAD,
	DIR_UPLOAD_LOC,
	DIR_CGI_HANDLER,
	DIR_UNKNOWN
};

struct	Directives
{
	public:
		Directives();
		Directives(const Directives& dir);
		~Directives();
		
		Directives& operator=(const Directives& dir);

		/* Getters */
		const std::string&					getRoot() const;
		const std::vector<std::string>&		getIndex() const;
		const bool&							getAutoIndex() const;
		const std::vector<std::string>&		getAllowedMethods() const;
		const size_t&						getMaxBodySize() const;
		const std::map<int, std::string>&	getErrorPages() const;
		bool								canUpload() const;
		std::string							getUploadPath() const;
		std::string							getPath() const;
		std::string							getAlias() const;

		/* Setters */
		void	setRoot(const std::string& root);
		void	setIndex(const std::vector<std::string>& );
		void	addIndex(const std::string&);
		void	setAutoIndex(const bool& autoindex);
		void	setAllowedMethods(const std::vector<std::string>& allowed_methods);
		void	addMethod(const std::string& method);
		void	setMaxBodySize(const size_t&);
		void	setErrorPages(const std::map<int, std::string>& error_pages);
		void	setUpload(bool);
		void	setUploadLoc(const std::string&);
		void	setPath(const std::string&);
		void	setAlias(const std::string&);

		void	setDirective(const enum e_directive_type directve_type, ConfigParser& conf);

		bool						isMethodAllowed(const std::string&) const;
		void	logs() const;

		const std::map<std::string, std::string>&   getCgiHandlers() const;

	private:
		std::string					_root;
		std::vector<std::string>	_index;
		bool						_autoindex;
		std::vector<std::string>	_allowed_methods;
		size_t						_client_max_body_size;
		std::map<int, std::string>	_error_pages;
		bool						_upload;
		std::string					_upload_loc;
		// for location
		std::string					_path;
		std::string					_alias;

		std::map<std::string, std::string>  _cgi_handlers;
};

e_directive_type getDirectiveType(const std::string& directive_str);
