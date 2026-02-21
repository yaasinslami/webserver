#ifndef RESPONSE
# define RESPONSE

#include "Directives.hpp"
#include "Request.hpp"
#include "Utils.hpp"

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
		void	generateError();
		/* Response reseter */
		void	clear();

		static std::string _sanitizePath(const std::string&);
		void _translatePath(const std::string uri);

		/* utils */
		std::string	_getMimeType(const std::string& path);

		private:
			void	_handleGet();
			void	_handlePost();
			void	_handleDelete();
			void	_handleDirectory();
			void	_serveFile();
};

#endif