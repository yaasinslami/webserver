#ifndef RESPONSE
# define RESPONSE

#include <string>
#include "Directives.hpp"
#include "Request.hpp"

class Response
{
	private:
		short& _code;
		std::string	_resource_path;
		std::string	_body_content;

		const Directives* _config;
	public:
		/* Constructors */
		Response(short& code);
		Response(const Response&);
		~Response();
		Response& operator=(const Response&);
		/* Getters */
		std::string	getBuffer() const;
		std::string	getFilePath() const;

		void setCode(short);
		void setConfig(const Directives*);
		void generate(const Request&);
		/* errors */
		void	generateError(const Directives*);
		/* Response reseter */
		void	clear();

		static std::string _sanitizePath(const std::string&);
		void _translatePath(const std::string uri);

		private:
			void	_handleGet();
			void	_handlePost();
			void	_handleDelete();
			void	_handleDirectory();
			void	_serveFile();
};

#endif