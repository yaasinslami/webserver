#include <exception>
#include <iostream>
#include "../inc/ConfigParser.hpp"

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

		std::cout << "Configuration '" << config_filename << "' loaded successfully." << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}

