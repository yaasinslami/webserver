#pragma once

#include "WebServer.hpp"
#include "Http.hpp"

class	Http;
class	Location;

class	ConfigParser
{
	public:
		ConfigParser(const std::string& filename);
		~ConfigParser();

		void	parse( void );

		/* Getters */
		const std::string&				getFileName() const;
		const std::vector<std::string>&	getTokens() const;
		const size_t&					getTokensIndex() const;
		Http&							getHttp();

		/* Setters */
		void	setFileName(const std::string& filename);
		void	setTokens(const std::vector<std::string> tokens);
		void	setTokensIndex(const size_t& tokens_size);

		// --- Token Utilities
		std::string peekToken();
		std::string consumeToken();
	
		private:
		std::string					_filename;
		std::ifstream				_file;
		std::vector<std::string>	_tokens;
		size_t						_token_index;
		Http						_http;

		// --- Helper Methods ---
		void tokenize(std::string& content);
		
		void		parseServerBlock();
		Location	parseLocationBlock(const Directives& s_directives);
};