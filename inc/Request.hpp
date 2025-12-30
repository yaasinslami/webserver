#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <map>

#define MAX_HEADER_SIZE 8192

// enum RequestState
// {
// 	STATE_READING_HEADERS,
// 	STATE_READING_BODY,
// 	STATE_COMPLETE,
// 	STATE_ERROR
// };

enum HttpMethod
{
	GET,
	POST,
	DELETE,
	NONE
};

enum RequestState
{
	STATE_START,			  // Waiting for first byte
	STATE_METHOD,			  // Reading "GET", "POST"...
	STATE_SPACE_BEFORE_URI,	  // Reading space after method
	STATE_URI,				  // Reading "/index.html"
	STATE_SPACE_BEFORE_PROTO, // Reading space after URI
	STATE_PROTOCOL,			  // Reading "HTTP/1.1"
	STATE_CR,				  // Saw '\r', expecting '\n' (Line End)
	STATE_HEADER_KEY,		  // Reading "Host", "User-Agent"
	STATE_HEADER_COLON,		  // Saw ':', waiting for value
	STATE_HEADER_VALUE,		  // Reading "localhost:8080"
	STATE_HEADER_CR,		  // Saw '\r' inside a header line
	STATE_CR_LF_CR,			  // Saw '\r\n\r', expecting '\n' (End of Headers)
	STATE_BODY,				  // Reading body data
	STATE_COMPLETE			  // Request finished
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
		std::string							_buffer;
		RequestState						_state;
		short								_m_index;
		size_t								_content_length;

		HttpMethod							_method;
		std::map<u_int8_t, std::string>		_method_str;
		std::string 						_path;
		std::string 						_version;
		std::map<std::string, std::string>	_headers;
		// std::string				_body;

		void	_parse_request_line(const std::string&);
		void	_parse_header(const std::string&);
		void	_check_body_complete(size_t);

};

#endif