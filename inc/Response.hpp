#ifndef RESPONSE
# define RESPONSE

#include "Directives.hpp"
#include "Request.hpp"
#include "Utils.hpp"

class Response
{
	private:
		short&	_code;
		int		_file_fd;
		off_t	_file_size;
		std::string	_resource_path;
		std::string	_headers;
		std::string	_body_content;

		const Directives* _config;
	public:
		/* Constructors */
		Response(short& code);
		Response(const Response&);
		~Response();
		Response& operator=(const Response&);
		/* Getters */
		int			getFd() const;
		std::string	getBuffer() const;
		std::string	getPhysicalPath() const;

		void setCode(short);
		void setConfig(const Directives*);

		void generate(const Request&);
		void	generateError();

		/* Response reseter */
		void	clear();

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