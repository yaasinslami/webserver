#include "ConfigParser.hpp"
#include "EpollManager.hpp"
#include <iostream>
#include <vector>

using namespace std;

static string get_config_path(int ac, char **av) 
{
	if (ac > 2)
		throw runtime_error("Usage: ./webserver [path/to/configuration_file.conf]");
	
	return (ac == 2 ? av[1] : "config/default.conf");
}

static int start_servers(vector<Server>& servers, EpollManager& poller)
{
	int started_count = 0;

	for (size_t i = 0; i < servers.size(); ++i)
	{
		if (servers[i].init()) 
		{
			if (poller.add_listener(&servers[i]))
				started_count++;
		}
		else
		{
			cerr << "Warning: Failed to start server on port " 
				 << servers[i].getPortStr() << endl;
		}
	}
	return started_count;
}

int main(int ac, char **av) 
{
	try 
	{
		string config_file = get_config_path(ac, av);
		ConfigParser parser(config_file);
		
		parser.parse();
		cout << "Configuration '" << config_file << "' loaded successfully." << endl;

		Http http = parser.getHttp();

		// http.logs();

		vector<Server>& servers = http.getServers(); 

		EpollManager poller;
		
		if (start_servers(servers, poller) == 0)
		{
			cerr << "Error: No servers could be started. Exiting." << endl;
			return 1;
		}
		else
			cout << "ALL Server started" << endl;

		cout << "Starting Event Loop..." << endl;
		poller.run_loop();
	}
	catch (const std::exception& e)
	{
		cerr << "Error: " << e.what() << endl;
		return 1;
	}

	return 0;
}
