#include "ConfigParser.hpp"
#include "EpollManager.hpp"
#include <iostream>
#include <vector>
#include <signal.h>

using namespace std;

int main(int ac, char **av) 
{
	signal(SIGPIPE, SIG_IGN);
	
	if (ac > 2) {
		std::cerr << "Usage: ./webserver [path/to/configuration_file.conf]\n";
		return 1;
	}
	try {
		ConfigParser parser(ac == 2 ? av[1] : "config/default.conf");
		parser.parse();
		Http http = parser.getHttp();
	
		EpollManager poller(http.getServers());
		cout << "Starting Event Loop..." << endl;
		poller.run_loop();
	}
	catch (const std::exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	}

	return 0;
}
