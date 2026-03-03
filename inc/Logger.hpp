#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"      /* CGI / SPECIAL */
#define CYAN    "\033[36m"      /* REQUESTS */

enum LogLevel {
	DEBUG,
	INFO,
	WARNING,
	ERROR
};

class Logger {
	public:
		static void log(LogLevel level, const std::string& messag, bool useTimestamp = true);
		static void init();
		static void close();

	private:
		static std::ofstream _logFile;
		static std::string _getTimestamp();
};

#endif // LOGGER_HPP