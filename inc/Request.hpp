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
	STATE_START,
	STATE_METHOD,
	STATE_SPACE_BEFORE_URI,
	STATE_URI_SLASH,
	STATE_URI_PATH,
	// STATE_URI_QUERY,
	STATE_PROTOCOL_H,
	STATE_PROTOCOL_HT,
	STATE_PROTOCOL_HTT,
	STATE_PROTOCOL_HTTP,
	STATE_PROTOCOL_SLASH,
	STATE_MAJOR_DIGIT,
	STATE_DOT_AFTER_DIGIT,
	STATE_MINOR_DIGIT,
	STATE_FIRST_CR,
	STATE_FIRST_LF,
	STATE_FIELD_START,
	STATE_FIELD_NAME,
	STATE_FIELD_VALUE,
	STATE_FIELD_END,
	STATE_COMPLETE
};

class	Request
{
	private:
		std::string		_buffer;
		RequestState	_state;
		short			_m_index;
		std::string		_tmp_buffer;
		size_t			_content_length;

		HttpMethod							_method;
		std::map<u_int8_t, std::string>		_method_str;
		std::string 						_path;
		std::string 						_version;
		std::map<std::string, std::string>	_headers;
		// std::string				_body;
		bool			_body_exist;

	public:
		Request();
		~Request();

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
		void	_parse_request_line(const std::string&);
		void	_parse_header(const std::string&);
		void	_check_body_complete(size_t);

		static bool	_is_token(unsigned char ch);
		void	_add_header();
		void	__headers();

};

#endif