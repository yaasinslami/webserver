#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>

enum RequestState
{
	STATE_READING_HEADERS,
	STATE_READING_BODY,
	STATE_COMPLETE
};

class	Request
{
	public:
		Request();
		~Request();

		std::string getMethod() const;
        std::string getPath() const;
        std::string getVersion() const;

		class	RequestException : public std::exception
		{
			private:
				int _status_code;

			public:
				RequestException(int code);
				int get_status_code() const;

				virtual const char *what() const throw();
		};

		bool	parsingCompleted() const;
		void	feed(const char* data, size_t size);

	private:
		std::string		_buffer;
		RequestState	_state;
		size_t			_content_length;

		std::string _method;
		std::string _path;
		std::string _version;
		// std::map<string, string>	_headers;
		// std::string				_body;

		void	parse(std::string raw);

};

#endif