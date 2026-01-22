#include "../inc/ConfigParser.hpp"
#include <csignal>

bool	g_running = true;

void	signalHandler(int signum) {
	(void)signum;
	g_running = false;
}

int main(int ac, char **av) 
{
	signal(SIGINT, signalHandler);

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
		http.logs();

		// lets start the real work
		// http.setup_servers();
		// http.run();

	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
