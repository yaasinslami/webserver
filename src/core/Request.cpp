#include "Request.hpp"
#include <sstream>
#include <vector>
#include <iostream>

/* FSM */
size_t	Request::feed(const std::string& data)
{
	size_t	i = 0;
	while (i < data.length())
	{
		unsigned char ch = data[i];
		switch (_state)
		{
			case STATE_START:
			{
				if (ch == '\r' || ch == '\n') {
					++i;
					continue ;
				}
				if (ch == 'G') _method = GET;
				else if (ch == 'P') _method = POST;
				else if (ch == 'D') _method = DELETE;
				else {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(501);
				}
				_state = STATE_METHOD;
				++i;
				continue ;
			}
			case STATE_METHOD:
			{
				if (ch != _method_str[_method][++_m_index]) {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(501);
				}
				if ((size_t)_m_index == _method_str[_method].length() - 1)
					_state = STATE_SPACE_BEFORE_URI;
				++i;
				continue ;
			}
			case STATE_SPACE_BEFORE_URI:
			{
				if (ch != ' ') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_URI_SLASH;
				++i;
				continue ;
			}
			case STATE_URI_SLASH:
			{
				if (ch != '/') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_URI_PATH;
				break ;
			}
			case STATE_URI_PATH:
			{
				if (ch == ' ') {
					_path = _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_PROTOCOL_H;
					++i;
					continue ;
				}
				break ;
			}
			case STATE_PROTOCOL_H:
			{
				if (ch != 'H') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HT;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_HT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HTT;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_HTT:
			{
				if (ch != 'T') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_HTTP;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_HTTP:
			{
				if (ch != 'P') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_PROTOCOL_SLASH;
				++i;
				continue ;
			}
			case STATE_PROTOCOL_SLASH:
			{
				if (ch != '/') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_MAJOR_DIGIT;
				++i;
				continue ;
			}
			case STATE_MAJOR_DIGIT:
			{
				if (ch != '1') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_DOT_AFTER_DIGIT;
				++i;
				continue ;
			}
			case STATE_DOT_AFTER_DIGIT:
			{
				if (ch != '.') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_MINOR_DIGIT;
				++i;
				continue ;
			}
			case STATE_MINOR_DIGIT:
			{
				if (ch != '1') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_FIRST_CR;
				++i;
				continue ;
			}
			case STATE_FIRST_CR:
			{
				if (ch != '\r') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_FIRST_LF;
				++i;
				continue ;
			}
			case STATE_FIRST_LF:
			{
				if (ch != '\n') {
					std::cerr << "Bad Request Line at: " << ch << std::endl;
					throw RequestException(400);
				}
				_state = STATE_FIELD_START;
				++i;
				continue ;
			}
			case STATE_FIELD_START:
			{
				if (ch == '\r') throw RequestException(400);

				if (Request::_is_token(ch)) {
					ch = tolower(ch);
					_state = STATE_FIELD_NAME;
				}
				else {
					throw RequestException(400);
				}
				break ;
			}
			case STATE_FIELD_NAME:
			{
				if (ch == '\r') {
					_state = STATE_FIELD_END;
					++i;
					continue ;
				}
				if (ch == ':') {// we have key:value
					_state = STATE_FIELD_VALUE;
				}
				else if (!Request::_is_token(ch)) {
					std::cerr << "Bad Header Key at: `" << ch << "`\n";
					throw RequestException(400);
				}
				// make ch lowercase
				ch = tolower(ch);
				break ;
			}
			case STATE_FIELD_VALUE:
			{
				if (ch == '\r') {
					_add_header();
					_state = STATE_FIELD_LF;
					++i;
					continue ;
				}
				if (!_is_valid_value(ch)) {
					throw RequestException(400);
				}
				break ;
			}
			case STATE_FIELD_LF:
			{
				if (ch != '\n') throw RequestException(400);

				_state = STATE_FIELD_NAME;
				++i;
				continue ;
			}
			case STATE_FIELD_END:
			{
				if (ch != '\n') throw RequestException(400);

				__headers();
				if (_body_exist) {
					if (_chunked_transfer) {
						_state = STATE_CHUNK_LENGTH;
					}
					else
						_state = STATE_BODY;
					++i;
					continue ;
				}
				else {
					_state = STATE_COMPLETE;
					++i;
					continue ;
				}
			}
			case STATE_BODY:
			{
				if (_body.size() < _content_length)
					_body += ch;
				if (_body.size() == _content_length)
					_state = STATE_COMPLETE;
				++i;
				continue ;
			}
			case STATE_CHUNK_LENGTH:
			{
				if (ch == '\r') {
					std::stringstream ss;
					ss << _tmp_buffer;
					_tmp_buffer.clear();
					ss >> std::hex >> _chunk_length;
					if (_chunk_length == 0 && _chunk_state == START)
						throw RequestException(400);
					else if (_chunk_length == 0) {
						_state = STATE_COMPLETE;
						++i;
						continue ;
					}
					this->_chunk_state = PAYLOAD_BODY;
					this->_state = STATE_CHUNK_LF;
					++i;
					continue ;
				}
				else if (!isxdigit(ch)) {
					std::cerr << "bad chunk-length" << std::endl;
					throw RequestException(400);
				}
				break ;
			}
			case STATE_CHUNK_FEED:
			{
				if (ch == '\r') {
					_state = STATE_CHUNK_LF;
					_chunk_state = LENGTH;
					++i;
					continue ;
				}

				if (_tmp_buffer.length() + 1 > _chunk_length)
					throw RequestException(400);
				break ;
			}
			case STATE_CHUNK_CR:
			{
				if (ch != '\r') throw RequestException(400);

				this->_state = STATE_CHUNK_LF;
				++i;
				continue ;
			}
			case STATE_CHUNK_LF:
			{
				if (ch != '\n') throw RequestException(400);

				if (_chunk_state == PAYLOAD_BODY)
					_state = STATE_CHUNK_FEED;
				
				if (_chunk_state == LENGTH) {
					this->_body += _tmp_buffer;
					_tmp_buffer.clear();
					_state = STATE_CHUNK_LENGTH;
				}
				++i;
				continue ;
			}
			case STATE_COMPLETE:
			{
				return i;
			}
		}
		_tmp_buffer += ch;
		++i;
	}
	return i;
}

void	Request::_add_header()
{
	size_t	delim = _tmp_buffer.find(':');
	std::string	key = _tmp_buffer.substr(0, delim);
	std::string value = _tmp_buffer.substr(delim + 1);

	if (key.empty()) {
		std::cerr << "key or value can't be empty!" << std::endl;
		throw RequestException(400);
	}

	// trim the space from the value
	// Key:  value  \r\n
	const std::string whitespace = " \t";
	size_t start = value.find_first_not_of(whitespace);
	size_t end = value.find_last_not_of(whitespace);

	if (start == std::string::npos)
		value = "";
	else {
		value = value.substr(start, end - start + 1);
	}

	if (_headers.count(key)) {
		if (key == "host" || key == "content-length") {
			throw RequestException(400);
		}

		_headers[key] += ", " + value;
	}
	else
		_headers[key] = value;

	std::cout << "Header Added: [" << key << "] -> [" << value << "]\n";
	_tmp_buffer.clear();
}

void	Request::__headers()
{
	if (_headers.count("content-length")) {
		std::string	value = _headers["content-length"];
	
		char	*endptr;
		const char* str = value.c_str();

		errno = 0;
	
		unsigned long len = std::strtoul(str, &endptr, 10);
		if (errno == ERANGE) {
			std::cerr << "content-length too large!";
			throw RequestException(400);
		}
		
		if (endptr == str || *endptr != '\0' || value.find('-') != std::string::npos) {
			std::cerr << "bad content-length" << std::endl;
			throw RequestException(400);
		}

		_content_length = (size_t)len;
		_body_exist = true;
	}

	if (_headers.count("transfer-encoding"))
	{
		if (_headers["transfer-encoding"].find("chunked") != std::string::npos) {
			_chunked_transfer = true;
		}
		_body_exist = true;
	}

	if (_headers.count("host")) {
		_server_name = _headers["host"];
	}
	else {
		std::cerr << "`Host` Header not found\n";
		throw RequestException(400);
	}

	if (_headers.count("connection") && _headers["connection"].find("close")) {
		_keep_alive = false;
	}

	// handle Content-Type ...

	std::cout << "we finish parsing of headers\n";
}

bool Request::_is_token(unsigned char c)
{
	if (isalnum(c))
		return true;

	std::string symbols = "!#$%&'*+-.^_`|~";
	return (symbols.find(c) != std::string::npos);
}

bool Request::_is_valid_value(unsigned char c)
{
	if (c == ' ' || c == '\t')
		return true;

	// Allow Visible ASCII (0x21 - 0x7E)
	if (c >= 0x21 && c <= 0x7E)
		return true;

	// Allow Extended ASCII (obs-text) (0x80 - 0xFF)
	if (c >= 0x80)
		return true;

	return false;
}

bool	Request::shouldKeepAlive() const
{
	return this->_keep_alive;
}

bool	Request::requestCompleted() const
{
	return _state == STATE_COMPLETE;
}

void	Request::resetState()
{
	_state = STATE_START;
	_m_index = 0;
	_content_length = 0;
	_chunked_transfer = false;
	_chunk_length = 0;
	_body_exist = false;
	_chunk_state = START;
	_keep_alive = true;
	_tmp_buffer.clear();
	_headers.clear();
}

Request::Request() :
	_state(STATE_START),
	_m_index(0),
	_content_length(0),
	_chunked_transfer(false),
	_chunk_length(0),
	_body_exist(false),
	_chunk_state(START),
	_keep_alive(true)
{
	_method_str[GET] = "GET";
	_method_str[POST] = "POST";
	_method_str[DELETE] = "DELETE";
}

Request::~Request() {}

std::string Request::getPath() const { return _path; }
std::string	Request::getHeader(const std::string& key) const
{
	return _headers.at(key);
}

Request::RequestException::RequestException(int code) : _status_code(code) {}
int Request::RequestException::get_status_code() const { return _status_code; }
const char* Request::RequestException::what() const throw() { return "Bad Request"; }


#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"

void Request::show() const
{
	std::cout << "\n"
			  << CYAN << "=== HTTP REQUEST DEBUG INFO ===" << RESET << std::endl;

	// 1. Request Line Info
	std::cout << YELLOW << "[Request Line]" << RESET << "\n";
	std::cout << "  Method:       " << GREEN;
	switch (_method)
	{
	case GET:
		std::cout << "GET";
		break;
	case POST:
		std::cout << "POST";
		break;
	case DELETE:
		std::cout << "DELETE";
		break;
	default:
		std::cout << "UNKNOWN";
	}
	std::cout << RESET << "\n";
	std::cout << "  Path:         " << _path << "\n";
	// std::cout << "  Query:        " << (_query.empty() ? "(empty)" : _query) << "\n";
	std::cout << "  Target Host:  " << (_server_name.empty() ? "(default)" : _server_name) << "\n";

	// 2. Headers
	std::cout << "\n"
			  << YELLOW << "[Headers]" << RESET << "\n";
	if (_headers.empty())
	{
		std::cout << "  (No Headers)\n";
	}
	else
	{
		std::map<std::string, std::string>::const_iterator it;
		for (it = _headers.begin(); it != _headers.end(); ++it) {
			std::cout << "  " << BLUE << it->first << RESET << ": " << it->second << "\n";
		}
	}

	// 3. Body Metadata (The specific logic you asked for)
	std::cout << "\n"
			  << YELLOW << "[Body Metadata]" << RESET << "\n";
	if (_body_exist)
	{
		if (_chunked_transfer)
		{
			std::cout << "  Mode:         " << RED << "Chunked Transfer Encoding" << RESET << "\n";
			std::cout << "  Decoded Size: " << _body.length() << " bytes\n";
		}
		else
		{
			std::cout << "  Mode:         " << GREEN << "Content-Length" << RESET << "\n";
			std::cout << "  Header Value: " << _content_length << "\n";
			std::cout << "  Actual Read:  " << _body.length() << " bytes\n";

			// Safety Warning if they don't match
			if (_content_length != _body.length())
			{
				std::cout << RED << "  WARNING: Body size mismatch!" << RESET << "\n";
			}
		}
	}
	else
	{
		std::cout << "  (No Body Present)\n";
	}

	// 4. The Actual Body Content
	// Only print if not empty, and maybe truncate if it's too huge
	if (_body_exist && !_body.empty())
	{
		std::cout << "\n"
				  << YELLOW << "[Body Content Preview]" << RESET << "\n";
		if (_body.length() > 1000)
		{
			std::cout << _body.substr(0, 1000) << "\n"
					  << RED << "...(truncated " << _body.length() - 1000 << " bytes)..." << RESET << "\n";
		}
		else
		{
			std::cout << _body << "\n";
		}
	}

	std::cout << CYAN << "===============================" << RESET << "\n"
			  << std::endl;
}