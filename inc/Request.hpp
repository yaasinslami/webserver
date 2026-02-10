#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>
#include <map>
#include <fstream>

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"

#define MAX_HEADER_SIZE 8192

enum	ChunkState
{
	LENGTH,
	PAYLOAD_BODY,
	CHUNK_END,
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
	STATE_CHUNK_CR,
	STATE_CHUNK_LF,
	STATE_CHUNK_FINAL_CR,
	STATE_CHUNK_FINAL_LF,
	STATE_CHUNK_DATA,
	STATE_BODY,
	STATE_COMPLETE
};

class	Request
{
	private:
		RequestState	_state;
		std::string	_tmp_buffer;
		size_t	_content_length;
		bool	_chunked_transfer;
		size_t	_chunk_length;

		std::string	_method;
		std::string	_uri_scheme;
		std::string	_uri_auth;
		std::string _uri_path;
		std::string _uri_query;
		std::string	_server_name;

		std::map<std::string, std::string>	_headers;

		std::string	_body_file_path;
		std::ofstream	_body_file;// file where the body will be stored
		size_t	_chunk_bytes_written;
		size_t	_total_body_bytes;
		bool	_body_exist;
		ChunkState	_chunk_state;
		bool	_keep_alive;
		bool	_headers_finished;


	public:
		Request();
		Request(const Request&);
		~Request();
		Request& operator=(const Request&);

		std::string	getMethod() const;
        std::string getPath() const;
		std::string	getServerName()const;
        std::string	getHeader(const std::string& key) const;
		size_t		getContentLength() const;
		size_t		getBodySize() const;

		bool		requestCompleted();
		bool		shouldKeepAlive() const;
		bool		headersFinished() const;


		// bool	parsingCompleted() const;
		ssize_t	feed(const char*,size_t);
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

		bool	_add_header();
		bool	__headers();

		static unsigned int _generate_random_id();
};

#endif