#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <map>

enum RequestState
{
	STATE_READING_HEADERS,
	STATE_READING_BODY,
	STATE_COMPLETE,
	STATE_ERROR
};

class	Request
{
	public:
		Request();
		~Request();

		std::string getMethod() const;
        std::string getPath() const;
        std::string getVersion() const;
		bool		requestCompleted() const;

		class	RequestException : public std::exception
		{
			private:
				int _status_code;

			public:
				RequestException(int code);
				int get_status_code() const;

				virtual const char *what() const throw();
		};

		// bool	parsingCompleted() const;
		void	feed(const char* chunk, size_t size);
		void	resetState();

	private:
		std::string		_buffer;
		RequestState	_state;
		size_t			_content_length;

		std::string					_method;
		std::string 				_path;
		std::string 				_version;
		std::map<std::string, std::string>	_headers;
		// std::string				_body;

		void	_parse_request_line(const std::string&);
		void	_parse_header(const std::string&);
		void	_check_body_complete(size_t);

};

#endif