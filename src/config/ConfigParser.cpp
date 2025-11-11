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

	std::cout << "Parsing complete. Found " << _server_configs.size() << " server blocks." << std::endl;
}

/*
 *} else if (directive == "error_page") {
            
            std::vector<int> codes;
            std::string path;
            
            // 1. Keep consuming tokens as long as they are numbers
            while (_token_index < _tokens.size() && isdigit(peekToken()[0])) {
                codes.push_back(std::atoi(consumeToken().c_str()));
            }
            
            // 2. The next token MUST be the path
            if (codes.empty()) {
                throw std::runtime_error("Error: No error code specified for 'error_page'.");
            }
            if (peekToken() == ";" || peekToken() == "") {
                 throw std::runtime_error("Error: No path specified for 'error_page'.");
            }
            path = consumeToken();

            // 3. The next token MUST be the semicolon
            if (consumeToken() != ";") {
                throw std::runtime_error("Error: Expected ';' after 'error_page' directive.");
            }

            // 4. Now, add all codes to the map
            for (size_t i = 0; i < codes.size(); ++i) {
                _http_config.error_pages[codes[i]] = path;
            }
        }
 *
 * #include <cstdlib> // For std::atoi, std::atol

void ConfigParser::parse(void)
{
    std::cout << "Start parsing tokens" << std::endl;
    
    // Loop until we have consumed all tokens in the file
    while (_token_index < _tokens.size()) {
        
        std::string directive = peekToken();
        consumeToken(); // Consume the directive (e.g., "server", "root")

        // --- Handle Server Blocks ---
        if (directive == "server") {
            // Delegate to the server block parser
            parseServerBlock();
        
        // --- Handle Global Directives ---
        } else if (directive == "root") {
            _http_config.root = consumeToken();
            if (consumeToken() != ";") {
                throw std::runtime_error("Error: Expected ';' after 'root' directive.");
            }
        
        } else if (directive == "index") {
            _http_config.index = consumeToken(); // Store the first value
            
            // Consume and ignore other values
            while (peekToken() != ";") {
                if (peekToken() == "" || peekToken() == "}") { // Check for bad syntax
                     throw std::runtime_error("Error: Missing ';' after 'index' directive.");
                }
                consumeToken(); // Ignore "index.htm", etc.
            }
            consumeToken(); // Consume the ";"
        
        } else if (directive == "client_max_body_size") {
            _http_config.client_max_body_size = std::atol(consumeToken().c_str());
            if (consumeToken() != ";") {
                throw std::runtime_error("Error: Expected ';' after 'client_max_body_size' directive.");
            }
        
        } else if (directive == "error_page") {
            int         error_code = std::atoi(consumeToken().c_str());
            std::string error_path = consumeToken();
            
            _http_config.error_pages[error_code] = error_path;
            
            if (consumeToken() != ";") {
                throw std::runtime_error("Error: Expected ';' after 'error_page' directive.");
            }

        // --- Handle Unknown Directives ---
        } else {
            throw std::runtime_error("Error: Unknown or unexpected directive in global scope: " + directive);
        }
    }

    // We've reached the end of the file.
    std::cout << "Parsing complete. Found " 
              << _http_config.server_configs.size() 
              << " server blocks." << std::endl;
}
 */

// ConfigParser::parseHttpBlock( void )
// {
// 	while (peekToken() != "}")
// 	{
// 		if (peekToken() == "")
// 			throw std::runtime_error("Unterminated http block. Expected '}'.");
//
// 		std::string	directive = peekToken();
// 		consumeToken();
//
// 		if (directive == "server")
// 			parseServerBlock();
// 		else if (directive == "root")
// 		{
// 			_http_config.root = consumeToken();
// 			if (consumeToken() != ";")
// 				throw std::runtime_error("Expected ';' after 'root' directive in http block.");
// 		}
// 		else if (directive == "index")
// 		{
// 			_http_config.index = consumeToken();
// 			if (consumeToken() != ";")
// 				throw std::runtime_error("Expected ';' after 'root' directive in http block.");
// 		}
// 		else if (directive == "client_max_body_size")
// 		{
// 			_http_config.client_max_body_size = std::atol(consumeToken().c_str());
// 			if (consumeToken() != ";") {
// 				throw std::runtime_error("Expected ';' after 'client_max_body_size' directive.");
// 			}
// 		}
// 		else if (directive == "error_pages")
// 		{
// 			_http_config.error_pages.first = first.first();
// 		}
// 	}
// }
