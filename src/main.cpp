#include "ConfigParser.hpp"
#include "EpollManager.hpp"
#include <iostream>

using namespace std;

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

		Http http = parser.getHttp();
		const vector<Server>& servers = http.getServers();
        EpollManager poller;
        bool         server_started = false;

        for (size_t i = 0; i < servers.size(); ++i)
        {
            if (servers[i].init()) 
            {
                if (poller.add_listener(&servers[i]))
                    server_started = true;
            }
            else
                std::cerr << "Failed to start server on port " 
                          << servers[i].getPortStr() << std::endl;
        }

        if (!server_started)
        {
            std::cerr << "No servers could be started. Exiting." << std::endl;
            return 1;
        }

        std::cout << "Starting Event Loop..." << std::endl;
        poller.run_loop();

	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
