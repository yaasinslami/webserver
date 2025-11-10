#include "../../inc/ConfigParser.hpp"
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

	if (consumeToken() != "http")
	{
		throw std::runtime_error("Expected 'http' block, but found: " + _tokens[_token_index - 1]);
	}
	// 3. Expect "{" right after "http"
	if (peekToken() == "") { // Check for "http" with no "{"
		throw std::runtime_error("Error: Expected '{' after 'http', but found end of file.");
	}
	if (consumeToken() != "{") {
		throw std::runtime_error("Error: Expected '{' after 'http', but found: " + _tokens[_token_index - 1]);
	}

	parseHttpBlock();

	// 5. Check for any "junk" tokens *after* the http block's closing '}'
	if (_token_index < _tokens.size()) {
		throw std::runtime_error("Error: Unexpected token after closing '}' of http block: " + peekToken());
	}

	std::cout << "Parsing complete. Found " << _server_configs.size() << " server blocks." << std::endl;
}

ConfigParser::parseHttpBlock( void )
{
	while (peekToken() != "}")
	{
		if (peekToken() == "")
			throw std::runtime_error("Unterminated http block. Expected '}'.");

		std::string	directive = peekToken();
		consumeToken();

		if (directive == "server")
			parseServerBlock();
		else if (directive == "root")
		{
			_http_config.root = consumeToken();
			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'root' directive in http block.");
		}
		else if (directive == "index")
		{
			_http_config.index = consumeToken();
			if (consumeToken() != ";")
				throw std::runtime_error("Expected ';' after 'root' directive in http block.");
		}
		else if (directive == "client_max_body_size")
		{
			_http_config.client_max_body_size = std::atol(consumeToken().c_str());
			if (consumeToken() != ";") {
				throw std::runtime_error("Expected ';' after 'client_max_body_size' directive.");
			}
		}
	}
}
