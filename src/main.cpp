#include <exception>
#include <iostream>
#include "../inc/ConfigParser.hpp"
#include "../inc/Http.hpp"


int main(int ac, char **av) 
{
	if (ac > 2)
	{
		std::cerr << "Error: Too many arguments." << std::endl;
		std::cerr << "Usage: ./webserver [path/to/configuration_file.conf]" << std::endl;
		return 1;
	}

	std::string config_filename = ac == 2 ? av[1] : "config/default.conf";

	try {
		ConfigParser	parser(config_filename);
		
		parser.parse();
		parser.debug();
		std::cout << "Configuration '" << config_filename << "' loaded successfully." << std::endl;
	
		Http = parser.getHttp();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}

// now here we should start our servers
// we should create our
// std::vector<int> _listening_fds;