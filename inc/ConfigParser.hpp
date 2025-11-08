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

struct	ServerConfig
{
	int							port;
	std::string					host;
	std::string					server_name;
	std::string					root;
	std::map<int, std::string>	error_pages;
	std::vector<LocationConfig>	locations;
	/***/
};

class	ConfigParser
{
	public:
		ConfigParser(const std::string& filename);
		~ConfigParser();

		void	parse( void );

		const std::vector<ServerConfig>& getServerConfigs() const;

	private:
		std::string					_filename;
		std::ifstream				_file;
		std::vector<std::string>	_tokens;
		std::vector<ServerConfig>	_server_configs;

		void tokenize(std::string& content);

		void parseServerBlock();
		void parseLocationBlock(ServerConfig& server);
};

