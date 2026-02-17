#ifndef RESPONSE
# define RESPONSE

#include <string>

class Response
{
	private:
		short& _code;

		std::string	_body_content;
		std::string	_file_path;
	public:
		/* Constructors */
		Response(short& code);
		Response(const Response&);
		~Response();
		Response& operator=(const Response&);
		/* Getters */
		std::string	getBuffer() const;
		std::string	getFilePath() const;
		/* errors */
		void	generateError();
		/* Response reseter */
		void	clear();
	private:

};

#endif