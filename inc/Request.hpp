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

enum	ChunkState
{
	LENGTH,
	LENGTH_LF,
	PAYLOAD_BODY,
	PAYLOAD_BODY_CR,
	PAYLOAD_BODY_LF,
};

enum RequestState
{
	STATE_START,
	STATE_METHOD,
	STATE_URI_START,
	STATE_URI_PATH,
	STATE_URI_QUERY,
	STATE_PROTOCOL,
	STATE_PROTOCOL_LF,
	STATE_FIELD_KEY,
	STATE_FIELD_VALUE,
	STATE_FIELD_LF,
	STATE_HEADERS_DONE,
	STATE_BODY_START,
	STATE_BODY,
	STATE_CHUNKED,
	STATE_COMPLETE,
	STATE_BAD_REQUEST
};

class	Request
{
	private:
		RequestState	_state;
		/* Request Line */
		std::string	_method;
		std::string	_uri_path;
		std::string	_uri_query;
		std::string	_protocol;
		/* Headers */
		std::string	_tmp_key;
		std::string	_tmp_value;
		std::map<std::string, std::string>	_headers;
		/* Body's Data informations */
		bool			_body_exist;
		size_t			_content_length;
		bool			_chunked_transfer;
		size_t			_chunk_length;
		ChunkState		_chunk_state;
		std::string		_body_file_path;
		std::ofstream	_body_file;
		size_t			_chunk_bytes_read;
		size_t			_total_body_bytes;

		std::string	_server_name;
		bool	_keep_alive;

		static const size_t MAX_METHOD_LEN = 10;
		static const size_t MAX_URI_LEN    = 1024;
		static const size_t MAX_HEADER_LEN = 512;


	public:
		Request();
		Request(const Request&);
		~Request();
		Request& operator=(const Request&);

		const RequestState&	getState() const;
		const std::string&	getMethod() const;
        const std::string&	getPath() const;
        const std::string&	getQuery() const;
		const std::string&	getServerName()const;
        const std::string&	getHeader(const std::string& key) const;
		const size_t&		getContentLength() const;
		const size_t&		getBodySize() const;
		const std::string&	getBodyPath() const;

		bool		requestCompleted();
		bool		shouldKeepAlive() const;
		void		resetState();

		void	feed(const char*,size_t);
		void	log() const;

	private:
		static bool	_is_unreserved(unsigned char);
		static bool	_is_sub_delim(unsigned char);
		static bool	_is_pchar(unsigned char);
		static bool	_is_query_char(unsigned char);
		static bool	_is_token(unsigned char);
		static bool	_is_valid_value(unsigned char);
		
		void _chunked_body(const char*, size_t, size_t&);

		bool	_add_header();
		bool	_validate_headers();

		static std::string _generate_random_id();
};

#endif