#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

class Utils
{
	public:
		static bool			set_nonblocking(int);
		static std::string getMimeType(const std::string&);
		static void*		memset(void *p, int c, size_t);
		static std::string	normalize_path(const std::string&);
		static std::string	normalize_uri(const std::string&);
		static std::string	urlDecode(const std::string&);
		static std::string	sanitizePath(const std::string &);
		static std::string	getStatusMessage(int);
		static long			getFileSize(const std::string&);
		static std::string resolve_path(const std::string&);
		static bool			Myaccess(const std::string&, short&, int, bool);
		static std::string generateRandomId();

		template <typename T>
		static std::string	toString(const T& value)
		{
			std::stringstream ss;
			ss << value;
			return ss.str();
		}
};
