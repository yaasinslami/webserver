#pragma once

#include "WebServer.hpp"
#include "Http.hpp"

class	ConfigParser
{
	public:
		ConfigParser(const std::string& filename);
		~ConfigParser();

		Http&	parse( void );
		void	debug(void);

		/* Getters */
		std::string						getFileName() const;
		const std::vector<std::string>&	getTokens() const;
		size_t							getTokensIndex() const;
		Http							getHttp() const;

		/* Setters */
		void	setFileName(const std::string& filename);
		void	setTokens(const std::vector<std::string> tokens);
		void	setTokensIndex(const size_t& tokens_size);

	private:
		std::string					_filename;
		std::ifstream				_file;
		std::vector<std::string>	_tokens;
		size_t						_token_index;
		Http						_http;

		// --- Helper Methods ---
		void tokenize(std::string& content);
		
		void parseHttpBlock();
		
		void parseServerBlock();
		void parseLocationBlock(ServerConfig& server);

		// --- Token Utilities
		std::string peekToken();
		std::string consumeToken();
};

