#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>

// Data structure for a "location" block
struct LocationConfig
{
	std::string					path;
	std::string					root;
	std::string					index;
	bool						autoindex;
	std::vector<std::string>	allowed_methods; // e.g., "GET", "POST"
	/***/

	LocationConfig() : autoindex(false) {} // Default values
};

struct ServerConfig
{
	int							port;
	std::string					host;
	std::string					server_name;
	std::string					root;
	std::map<int, std::string>	error_pages;
	std::vector<LocationConfig>	locations;
	/***/
};

struct HttpConfig
{
	std::vector<ServerConfig> server_configs; // All servers live inside http
	std::string               root; // Global default root
	std::string               index; // Global default index
	long                      client_max_body_size;
	// ... add other http-level directives ...

	// Set global defaults
	HttpConfig() : root("/var/www/html"), client_max_body_size(1048576) {} // 1M default
};

class	ConfigParser
{
	public:
		ConfigParser(const std::string& filename);
		~ConfigParser();

		void	parse( void );

		const HttpConfig& getHttpConfig() const;

	private:
		std::string					_filename;
		std::ifstream				_file;
		std::vector<std::string>	_tokens;
		size_t						_token_index;

		HttpConfig                  _http_config;

		 // --- Helper Methods ---
		void tokenize(std::string& content);
		
		void parseHttpBlock();
		
		void parseServerBlock();
		void parseLocationBlock(ServerConfig& server);

		// --- Token Utilities
		std::string peekToken();
		std::string consumeToken();
};

