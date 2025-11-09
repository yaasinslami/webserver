#include "../../inc/ConfigParser.hpp"
#include <sstream>
#include <stdexcept>

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

		if (peekToken() != "http")
		{
				if (peekToken() == "") throw std::runtime_error("Error: Config file is empty.");
			throw std::runtime_error("Error: Expected 'http' block, but found: " + peekToken());
		}
	}


	std::cout << "Parsing complete. Found " << _server_configs.size() << " server blocks." << std::endl;
}

/*
 *
 * void ConfigParser::parse() {
    std::cout << "Starting parser..." << std::endl;

    // Loop while there are tokens left
    while (_token_index < _tokens.size()) {
        
        std::string token = peekToken();

        if (token == "server") {
            consumeToken(); // Consume "server" token
            parseServerBlock();
        } else {
            throw std::runtime_error("Error: Unexpected token in global scope: " + token);
        }
    }
    std::cout << "Parsing complete. Found " << _server_configs.size() << " server blocks." << std::endl;
}
 */
