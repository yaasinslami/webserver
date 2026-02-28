#include "ConfigParser.hpp"
#include "EpollManager.hpp"
#include <iostream>
#include <vector>
#include <signal.h>

volatile sig_atomic_t g_running = 1;

void handle_signal(int signum) {
	(void)signum;
	g_running = 0;
}

int main(int ac, char **av) 
{
	// signal(SIGPIPE, SIG_IGN);

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	if (ac > 2) {
		std::cerr << "Usage: ./webserver [path/to/configuration_file.conf]\n";
		return 1;
	}
	try {
		ConfigParser parser(ac == 2 ? av[1] : "config/default.conf");
		parser.parse();
		EpollManager poller(parser.getHttp().getServers());
		std::cout << "Starting Event Loop..." << std::endl;
		poller.run_loop();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
