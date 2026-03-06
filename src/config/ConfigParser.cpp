#include "ConfigParser.hpp"
#include "Server.hpp"
#include "Location.hpp"
#include "Http.hpp"
#include "Directives.hpp"
#include <stdexcept>
#include <algorithm>

/* Parsers */
void	ConfigParser::tokenize(std::string& content)
{
	std::stringstream	lines(content);
	std::string	processed_content;
	std::string	line;

	while (std::getline(lines, line)) {
		size_t	cmnt_pos = line.find('#');
		if (cmnt_pos != std::string::npos) line = line.substr(0, cmnt_pos);
		processed_content += line + "\n";
	}

	std::string	spaced_content;
	for (size_t i = 0; i < processed_content.length(); i++) {
		if (processed_content[i] == '{' || processed_content[i] == '}' || processed_content[i] == ';') {
			spaced_content += ' ';
			spaced_content += processed_content[i];
			spaced_content += ' ';
		}
		else spaced_content += processed_content[i];
	}

	std::stringstream	ss_tokens(spaced_content);
	std::string			tokens;
	while (ss_tokens >> tokens) _tokens.push_back(tokens);

}
void	ConfigParser::parse( void )
{
	while (getTokensIndex() < getTokens().size())
	{
		e_directive_type directive_type = getDirectiveType(consumeToken());

		if (directive_type == DIR_SERVER) {
			parseServerBlock();
			continue ;
		}
		else if (_reg_directive(directive_type)) {
			_http.getDirectives().setDirective(directive_type, *this);
		}
		else if (directive_type == DIR_UNKNOWN)
			throw std::runtime_error("Unknown or unexpected directive in global scope");
		
		if (consumeToken() != ";")
			throw std::runtime_error("Missing ';' in the Global Section.");
	}
	if (getHttp().getServers().empty())
		throw std::runtime_error("Configuration error: No 'server' blocks defined.");
	
}
void	ConfigParser::parseServerBlock( void )
{
	Server	server(getHttp().getDirectives());

	if (consumeToken() != "{") {
		throw std::runtime_error("Expected '{' after 'server' directive.");
	}

	while (peekToken() != "}")
	{
		e_directive_type directive_type = getDirectiveType(consumeToken());


		if (directive_type == DIR_LOCATION) {
			server.addLocation(parseLocationBlock(server.getDirectives()));
			continue ;
		}
		else if (directive_type == DIR_LISTEN)
		{
			std::string	listenStr = consumeToken();
		
			size_t	colonPos = listenStr.find(':');

			if (colonPos != std::string::npos)//possibly found both interface:port
			{
				server.setInterface(listenStr.substr(0, colonPos));
				server.setPort(listenStr.substr(colonPos + 1));
			}
			else
			{
				bool is_numeric = true;
				for (size_t i = 0; i < listenStr.length(); ++i)
				{
					if (!std::isdigit(listenStr[i]))
					{
						is_numeric = false;
						break;
					}
				}

				if (is_numeric) {
					server.setPort(listenStr);
				}
				else {
					server.setInterface(listenStr);
				}
			}
		}
		else if (directive_type == DIR_SERVER_NAME) {
			while (peekToken() != ";") {
				server.addServerName(consumeToken());
			}
		}
		else if (_reg_directive(directive_type)) {
			server.getDirectives().setDirective(directive_type, *this);
		}
		else
			throw std::runtime_error("Unknown or unexpected directive in server block");
		
		if (consumeToken() != ";")
			throw std::runtime_error("Missing ';' In a Server Section.");
	}

	server.validate();
	std::sort(server.getLocations().begin(), server.getLocations().end(), Location::compareLocations);
	consumeToken();// consume the closing "}"
	getHttp().addServer(server);
}
Location	ConfigParser::parseLocationBlock(const Directives& s_directives)
{
	Location	location(s_directives);

	location.setPath(consumeToken());

	if (consumeToken() != "{")
		throw std::runtime_error("Expected '{' after 'location' directive.");

	while (peekToken() != "}")
	{
		e_directive_type directive_type = getDirectiveType(consumeToken());

		if (directive_type == DIR_ALIAS) {
			location.setAlias(consumeToken());
		}
		else if (_reg_directive(directive_type)) {
			location.getDirectives().setDirective(directive_type, *this);
		}
		else
			throw std::runtime_error("Unknown or unexpected directive in 'Location' block");

		if (consumeToken() != ";")
			throw std::runtime_error("Missing ';' In a Location Section.");
	}

	consumeToken();
	return location;
}
/* Tokens */
std::string	ConfigParser::peekToken( void )
{
	if (getTokensIndex() >= getTokens().size()) return "";
	return _tokens[_token_index];
}
std::string	ConfigParser::consumeToken( void )
{
	if (getTokensIndex() >= getTokens().size())
		throw std::runtime_error("Unexpected end of file. Expected more tokens.");

	return _tokens[_token_index++];
}
/* Random stuff */
bool ConfigParser::_reg_directive(const enum e_directive_type directive_type)
{
	if (directive_type == DIR_ROOT || \
		directive_type == DIR_INDEX || \
		directive_type == DIR_AUTO_INDEX || \
		directive_type == DIR_ALLOWED_METHODS || \
		directive_type == DIR_MAX_BODY_SIZE || \
		directive_type == DIR_ERROR_PAGES || \
		directive_type == DIR_UPLOAD || \
		directive_type == DIR_UPLOAD_LOC || \
		directive_type == DIR_CGI_HANDLER || \
		directive_type == DIR_UPLOAD || \
		directive_type == DIR_UPLOAD_LOC)

		return true;
	return false;
}
/* Getters */
const std::vector<std::string>&	ConfigParser::getTokens() const { return this->_tokens; }
const size_t&	ConfigParser::getTokensIndex() const {return this->_token_index; }
Http& ConfigParser::getHttp() { return _http; }
/* Setters */
void	ConfigParser::setTokens(const std::vector<std::string> tokens) { this->_tokens = tokens; }
void	ConfigParser::setTokensIndex(const size_t& tokens_size) {this->_token_index = tokens_size; }
/* Constructors */
ConfigParser::ConfigParser(const std::string& filename) :
	_token_index(0)
{

	struct stat file_stat;
	if (stat(filename.c_str(), &file_stat) != 0) throw std::runtime_error("Config file does not exist: " + filename);
	if (!S_ISREG(file_stat.st_mode)) throw std::runtime_error("Config path is not a regular file: " + filename);

	std::ifstream file;
	file.open(filename.c_str());
	if (!file.is_open()) throw std::runtime_error("Could not open config file: " + filename);

	std::stringstream	ss;
	ss << file.rdbuf();
	file.close();
	std::string	file_content = ss.str();
	tokenize(file_content);
}
ConfigParser::~ConfigParser() {}
