#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

class Utils
{
	public:
		static bool			set_nonblocking(int socket);
		static std::string getMimeType(const std::string&);
		static void*		memset(void *p, int c, size_t len);
		static std::string	normalize_path(const std::string& path);
		static std::string normalize_uri(const std::string&);
		static std::string urlDecode(const std::string&);
		static std::string sanitizePath(const std::string &);
		static std::string getStatusMessage(int code);
		static long			getFileSize(const std::string& filename);
		static std::string resolve_path(const std::string& input_path);
		static std::string generateRandomId();

		template <typename T>
		static std::string	toString(const T& value)
		{
			std::stringstream ss;
			ss << value;
			return ss.str();
		}
};
