#include "../../inc/ConfigParser.hpp"
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>

ConfigParser::ConfigParser(const std::string& filename) : _filename(filename)
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

	// a debug line
    // for (size_t i = 0; i < _tokens.size(); ++i) {
    //     std::cout << "Token " << i << ": " << _tokens[i] << std::endl;
    // }
	std::cout << "Tokenizer finished" << std::endl;
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

	while (_token_index < _tokens.size())
	{
		std::string	directive = peekToken();
		consumeToken();
		
		if (directive == "server")
			parseServerBlock();
		else if (directive == "root")
		{
			_http_config.root = consumeToken();
			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'root' directive.");
		}
		else if (directive == "index")
		{
			_http_config.index = consumeToken();

			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'index' directive.");
		}
		else if (directive == "client_max_body_size")
		{
			_http_config.client_max_body_size = std::atol(consumeToken().c_str());
			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'client_max_body_size' directive.");
		}
		else if (directive == "error_pages")
		{
			std::vector<int>	codes;
			std::string			path;

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
				_http_config.error_pages[codes[i]] = path;
			}
		}
		else
			throw std::runtime_error("Error: Unknown or unexpected directive in global scope: " + directive);
	}

	std::cout << "Parsing complete. Found " << _http_config.server_configs.size() << " server blocks." << std::endl;
}

void	ConfigParser::parseServerBlock( void )
{
	ServerConfig	server;

	if (consumeToken() != "{")
		throw std::runtime_error("Expected '{' after 'server' directive.");

	while (peekToken() != "}")
	{
		std::string	directive = consumeToken();

		if (directive == "location")
			parseLocationBlock(server);
		else if (directive == "listen")
		{
			server.port = std::atoi(consumeToken().c_str());
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'listen' directive.");
		}
		else if (directive == "server_name")
		{
			server.server_name = consumeToken();
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'server_name' directive.");
		}
		else if (directive == "root")
		{
			server.root = consumeToken();
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'root' directive.");
		}
		else
			throw std::runtime_error("Unknown or unexpected directive in server block: " + directive);

		consumeToken();

		_http_config.server_configs.push_back(server);
	}
}

void	ConfigParser::parseLocationBloc(ServerConfig& server)
{
	LocationConfig	location;

	if (consumeToken() != "{")
		throw std::runtime_error("Expected '{' after 'server' directive.");

	while (peekToken() != "}")
	{
		std::string	directive = consumeToken();

		if (directive == "root")
		{
			location.root = consumeToken();
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'root' directive.");
		}
		else if (directive == "index")
		{
			location.index = consumeToken();
			if (consumeToken() != ";")
				throw std::runtime_error("Missing ';' after 'index' directive.");
		}
		else if (directive == "allowed_methods")
		{
			while (peekToken() != ";")
			{
				std::string	method = consumeToken();
				if (method != "GET" && method != "POST" && method != "DELETE")
					throw std::runtime_error("Unknown or Unexpected Method in 'allowed_methods' directive");
				location.allowed_methods.push_back(peekToken());
			}
		}
		else
			throw std::runtime_error("Unknown or unexpected directive in 'Location' block: " + directive);

	}

	server.locations.push_back(location);
}
