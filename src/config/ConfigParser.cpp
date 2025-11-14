#include "../../inc/ConfigParser.hpp"
#include "../../inc/Server.hpp"
#include "../../inc/Location.hpp"
#include <stdexcept>

ConfigParser::ConfigParser(const std::string& filename) : _filename(filename), _token_index(0)
{
	this->_file.open(_filename.c_str());
    if (!this->_file.is_open())
		throw std::runtime_error("Could not open config file: " + _filename);

	std::stringstream	ss;
	ss << _file.rdbuf();

	std::string	file_content = ss.str();

	tokenize(file_content);
}

ConfigParser::~ConfigParser()
{
	if (_file.is_open())
		_file.close();
}

void	ConfigParser::tokenize(std::string& content)
{
	std::stringstream	lines(content);
	std::string	processed_content;
	std::string	line;

	while (std::getline(lines, line))
	{
		size_t	cmnt_pos = line.find('#');
		if (cmnt_pos != std::string::npos)
			line = line.substr(0, cmnt_pos);

		processed_content += line + "\n";
	}

	std::string	spaced_content;
	for (size_t i = 0; i < processed_content.length(); i++)
	{
		if (processed_content[i] == '{' || processed_content[i] == '}' || processed_content[i] == ';')
		{
			spaced_content += ' ';
			spaced_content += processed_content[i];
			spaced_content += ' ';
		}
		else
		{
			spaced_content += processed_content[i];
		}
	}

	std::stringstream	ss_tokens(spaced_content);
	std::string			tokens;
	while (ss_tokens >> tokens)
		_tokens.push_back(tokens);

	if (_tokens.size() < 1)
		throw std::runtime_error("Config file is empty");
	// a debug line
	// for (size_t i = 0; i < _tokens.size(); ++i) {
	//     std::cout << "Token " << i << ": " << _tokens[i] << std::endl;
	// }
	// std::cout << "Tokenizer finished" << std::endl;
}

std::string	ConfigParser::peekToken( void )
{
	if (_token_index >= _tokens.size())
		return "";
	return _tokens[_token_index];
}

std::string	ConfigParser::consumeToken( void )
{
	if (_token_index >= _tokens.size())
		throw std::runtime_error("Unexpected end of file. Expected more tokens.");

	return _tokens[_token_index++];
}

void	ConfigParser::parse( void )
{
	std::cout << "Start parsing tokens" << std::endl;

	while (getTokensIndex() < getTokens().size())
	{
		std::string	directive = peekToken();
		consumeToken();
		
		if (directive == "server") {
			parseServerBlock();
		}
		else if (directive == "root")
		{
			_http.setRoot(consumeToken());
			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'root' directive.");
		}
		else if (directive == "index")
		{
			_http.setIndex(consumeToken());

			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'index' directive.");
		}
		else if (directive == "client_max_body_size")
		{
			_http.setMaxBodySize(std::atol(consumeToken().c_str()));
			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'client_max_body_size' directive.");
		}
		else if (directive == "error_pages")
		{
			std::vector<int>			codes;
			std::string					path;
			std::map<int, std::string>	mp;

			while (_token_index < _tokens.size() && isdigit(peekToken()[0])) {
				codes.push_back(std::atoi(consumeToken().c_str()));
			}

			if (codes.empty())
				throw std::runtime_error("No error code specified for 'error_page'.");

			if (peekToken() == ";" || peekToken() == "")
				throw std::runtime_error("No path specified for 'error_page'.");

			path = consumeToken();

			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'error_page' directive.");

			for (size_t i = 0; i < codes.size(); ++i) {
				mp[codes[i]] = path;
			}
			_http.setErrorPages(mp);
		}
		else
			throw std::runtime_error("Unknown or unexpected directive in global scope: " + directive);
	}
	if (_http.getServers().empty())
		throw std::runtime_error("Configuration error: No 'server' blocks defined.");
	std::cout << "Parsing complete. Found " << _http.getServers().size() << " server blocks." << std::endl;
}

void	ConfigParser::parseServerBlock( void )
{
	Server	server;

	server.setRoot(_http.getRoot());
	server.setIndex(_http.getIndex());
	server.setMaxBodySize(_http.getMaxBodySize());
	server.setErrorPages(_http.getErrorPages());

	if (consumeToken() != "{") {
		throw std::runtime_error("Expected '{' after 'server' directive.");
	}

	while (peekToken() != "}")
	{
		std::string	directive = consumeToken();

		if (directive == "location")
			parseLocationBlock(server);
		else if (directive == "listen")
		{
			std::string	listenStr = consumeToken();
		
			size_t	colonPos = listenStr.find(':');

			if (colonPos != std::string::npos)
			{
				server.setInterface(listenStr.substr(0, colonPos));
				std::string	portStr = listenStr.substr(colonPos + 1);
				server.setPort(std::atoi(portStr.c_str()));
			}
			else//we didnt find ":" => interfce or port ?
			{
				if (listenStr.find('.') != std::string::npos)// we found an IP
					server.setInterface(listenStr);
				else// we have a port
					server.setPort(std::atoi(listenStr.c_str()));
			}

			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'listen' directive.");
		}
		else if (directive == "server_name")
		{
			server.setServerName(consumeToken());
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'server_name' directive.");
		}
		else if (directive == "root")
		{
			server.setRoot(consumeToken());
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'root' directive.");
		}
		else
			throw std::runtime_error("Unknown or unexpected directive in server block: " + directive);
	}

	consumeToken();
	_http.addServer(server);
}

void	ConfigParser::parseLocationBlock(Server& server)
{
	Location	location(server);

	location.setPath(consumeToken());

	if (consumeToken() != "{")
		throw std::runtime_error("Expected '{' after 'location' directive.");

	while (peekToken() != "}")
	{
		std::string	directive = consumeToken();

		if (directive == "root")
		{
			// if (!location.getAlias().empty())
			// 	throw std::runtime_error("Configuration error: 'alias' and 'root' directives are mutually exclusive.");	
			location.setRoot(consumeToken());
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'root' directive.");
		}
		else if (directive == "alias")
		{
			// if (!location.getRoot().empty())
			// 	throw std::runtime_error("Configuration error: 'alias' and 'root' directives are mutually exclusive.");
			location.setAlias(consumeToken());
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'alias' directive.");
		}
		else if (directive == "index")
		{
			location.setIndex(consumeToken());
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'index' directive.");
		}
		else if (directive == "allowed_methods")
		{
			while (peekToken() != ";")
			{
				std::string	method = consumeToken();
				if (method != "GET" && method != "POST" && method != "DELETE")
					throw std::runtime_error("Unknown or Unexpected Method in 'allowed_methods' directive.");
				location.addMethod(method);
			}
			consumeToken();
		}
		else if (directive == "autoindex")
		{
			std::string	autoindex = consumeToken();
			if (autoindex == "on")
				location.setAutoIndex(true);
			else if (autoindex == "off")
				location.setAutoIndex(false);
			else
				throw std::runtime_error("Unknown or unexpected in 'autoindex' directive.");
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'autoindex' directive.");
		}
		else
			throw std::runtime_error("Unknown or unexpected directive in 'Location' block: " + directive);

	}

	consumeToken();
	server.addLocation(location);
}

const std::string&	ConfigParser::getFileName() const
{
	return this->_filename;
}

const std::vector<std::string>&	ConfigParser::getTokens() const
{
	return this->_tokens;
}

const size_t&	ConfigParser::getTokensIndex() const
{
	return this->_token_index;
}

const Http& ConfigParser::getHttp() const
{
	return _http;
}

void	ConfigParser::setFileName(const std::string& filename)
{
	this->_filename = filename;
}

void	ConfigParser::setTokens(const std::vector<std::string> tokens)
{
	this->_tokens = tokens;
}