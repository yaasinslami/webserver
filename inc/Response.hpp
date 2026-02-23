#ifndef RESPONSE
# define RESPONSE

#include "Directives.hpp"
#include "Request.hpp"
#include "Utils.hpp"

class Response
{
	private:
		short&	_code;
		size_t	_file_size;
		std::string	_content_type;
		std::string	_resource_path;
		std::string	_headers;
		std::string	_body_content;
		std::string	_redirect_location;
		bool		_is_static_file;

		const Directives* _config;
	public:
		/* Constructors */
		Response(short& code);
		Response(const Response&);
		~Response();
		Response& operator=(const Response&);
		/* Getters */
		int				getFd() const;
		std::string&	getBuffer();
		std::string&	getHeaderBuffer();
		std::string		getPhysicalPath() const;
		/* Setters */
		void setCode(short);
		void setConfig(const Directives*);

		void setPath(std::string&);

		void generate(const Request&);
		void	generateError();
		void	_generateAutoIndex(const std::string&);
		void	_buildHeaders();

		/* Response reseter */
		void	clear();
		void	clearFilePath();

		void translatePath(const std::string& uri);
	private:
		/* GET */
		void	_handleGet(const std::string&);
		void	_handleDirectory(const std::string&);
		void	_serveFile();
		 /* POST */
		void	_handlePost();
		 /* DELETE */
		void	_handleDelete();
};

#endif