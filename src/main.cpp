#include "ConfigParser.hpp"
#include "EpollManager.hpp"
#include <iostream>
#include <vector>
#include <signal.h>
#include "Logger.hpp"

volatile sig_atomic_t g_running = 1;

void handle_signal(int signum) {
	(void)signum;
	g_running = 0;
}

int main(int ac, char **av) 
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	Logger::init();
	if (ac > 2) {
		std::cerr << "Usage: ./webserver [path/to/configuration_file.conf]\n";
		return 1;
	}
	try {
		ConfigParser parser(ac == 2 ? av[1] : "configs/default.conf");
		parser.parse();

		EpollManager poller(parser.getHttp().getServers());
		poller.run_loop();
	}
	catch (const std::exception& e) {
		Logger::log(ERROR, e.what());
	}

	Logger::close();
	return 0;
}
