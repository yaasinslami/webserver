#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <map>

#define MAX_HEADER_SIZE 8192

enum HttpMethod
{
	GET,
	POST,
	DELETE,
	NONE
};

enum	ChunkState
{
	START,
	LENGTH,
	PAYLOAD_BODY,
	FINISH,
};

enum RequestState
{
	STATE_START,
	STATE_METHOD,
	STATE_SPACE_BEFORE_URI,
	STATE_URI_START,
	STATE_URI_SCHEME,
	STATE_SCHEME_FIRST_SLASH,
	STATE_SCHEME_SECOND_SLASH,
	STATE_URI_AUTH,
	STATE_URI_PATH,
	STATE_URI_QUERY,
	STATE_URI_FRAGMENT,
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
	// STATE_FIELD_CR,
	STATE_FIELD_LF,
	STATE_CHUNK_LENGTH,
	STATE_CHUNK_FEED,
	STATE_CHUNK_CR,
	STATE_CHUNK_LF,
	STATE_BODY,
	STATE_COMPLETE
};

class	Request
{
	private:
		RequestState	_state;
		short			_m_index;
		std::string		_tmp_buffer;
		size_t			_content_length;
		bool			_chunked_transfer;
		size_t			_chunk_length;

		HttpMethod							_method;
		std::map<u_int8_t, std::string>		_method_str;
		std::string							_uri_scheme;
		std::string							_uri_auth;
		std::string 						_uri_path;
		std::string 						_uri_query;
		std::string							_server_name;
		std::map<std::string, std::string>	_headers;
		std::string			_body;
		bool				_body_exist;
		ChunkState			_chunk_state;
		bool				_keep_alive;

	public:
		Request();
		~Request();

		std::string	getMethod() const;
        std::string getPath() const;
		std::string	getServerName()const;
        std::string	getHeader(const std::string& key) const;
		const std::string	&getBody() const;
		bool		requestCompleted() const;
		bool		shouldKeepAlive() const;

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
		size_t	feed(const std::string&);
		void	resetState();

		void	show() const;

	private:
		static bool	_is_scheme_char(unsigned char);
		static bool	_is_unreserved(unsigned char);
		static bool	_is_sub_delim(unsigned char);
		static bool	_is_pchar(unsigned char);
		static bool	_is_query_char(unsigned char);
		static bool	_is_token(unsigned char);
		static bool	_is_valid_value(unsigned char);
		void	_add_header();
		void	__headers();

};

#endif