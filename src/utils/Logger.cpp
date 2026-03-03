#include "Logger.hpp"
#include <iostream>
#include <ctime>

std::ofstream Logger::_logFile;

void Logger::log(LogLevel level, const std::string& message, bool useTimestamp) {
	std::string color;
	std::string levelName;
	std::ostream* output = &std::cout;

	switch (level) {
		case ERROR:
			color = RED;
			levelName = "[ERROR]";
			output = &std::cerr;
			break;
		case WARNING:
			color = YELLOW;
			levelName = "[WARN ]";
			output = &std::cerr;
			break;
		case DEBUG:
			color = BLUE;
			levelName = "[DEBUG]";
			break;
		case INFO:
			color = GREEN;
			levelName = "[INFO ]";
			break;
	}

	std::string prefix = "";
	if (useTimestamp) {
		prefix = _getTimestamp() + " ";
	}

	*output << color << prefix << levelName << " " << message << RESET << std::endl;

	if (_logFile.is_open()) {
		_logFile << prefix << levelName << " " << message << std::endl;
		_logFile.flush();
	}
}

std::string Logger::_getTimestamp() {
	char buffer[80];
	time_t now = time(0);
	struct tm *tstruct = localtime(&now);

	strftime(buffer, sizeof(buffer), "[%Y-%m-%d %H:%M:%S]", tstruct);

	return std::string(buffer);
}

void Logger::init() {
	_logFile.open("/tmp/webserver.log", std::ios::app);
	
	if (!_logFile.is_open())
		throw std::runtime_error("Failed to open log file for writing.");
}

void Logger::close() {
	if (_logFile.is_open()) {
		_logFile.close();
	}
}