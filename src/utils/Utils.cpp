#include "Utils.hpp"

bool	Utils::set_nonblocking(int socket)
{
	if (fcntl(socket, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << "Error: fcntl F_SETFL O_NONBLOCK failed" << std::endl;
		return (false);
	}

	if (fcntl(socket, F_SETFD, FD_CLOEXEC) == -1)
	{
		std::cerr << "Error: fcntl F_SETFD FD_CLOEXEC failed" << std::endl;
		return (false);
	}

	return (true);
}

void*	Utils::memset(void *p, int c, size_t n)
{
	unsigned char *ptr = (unsigned char *)p;
	size_t i = 0;

	while (i < n)
	{
		ptr[i] = (unsigned char)c;
		i++;
	}

	return (ptr);
}

std::string Utils::urlDecode(const std::string& str) 
{
	std::string result;
	result.reserve(str.length());

	for (size_t i = 0; i < str.length(); ++i) {
		if (str[i] == '%') 
		{
			// Make sure there are at least 2 characters after the '%'
			if (i + 2 < str.length()) {
				int value;
				std::istringstream is(str.substr(i + 1, 2));
				// Read the two characters as a hex number
				if (is >> std::hex >> value) {
					result += static_cast<char>(value);
					i += 2; // Skip the two hex characters we just read
				}
				else result += '%'; // Fallback if it wasn't valid hex
			}
			else result += '%'; // Fallback if string ends abruptly
		} 
		else if (str[i] == '+') result += ' ';
		else result += str[i];
	}

	return result;
}

long Utils::getFileSize(const std::string& filename)
{
	struct stat st;
	if (stat(filename.c_str(), &st) == 0)
		return st.st_size;
	return -1;
}

std::string Utils::resolve_path(const std::string& input_path)
{
	char *pwd = getenv("PWD");
	std::string	absolute_res;

	if (!pwd) return input_path;

	std::string base_path(pwd);
	if (input_path[0] == '/')
		absolute_res = normalize_uri(input_path);
	else if (input_path.compare(0, 2, "./") == 0)
		absolute_res = normalize_uri(base_path + "/" + input_path.substr(2));
	else
		absolute_res = normalize_uri(base_path + "/" + input_path);

	return absolute_res;
}

static std::vector<std::string>	resolve_components(const std::string& input)
{
	std::vector<std::string> components;
	std::stringstream ss(input);
	std::string comp;

	while (std::getline(ss, comp, '/')) {
		if (comp.empty() || comp == ".") continue;
		if (comp == "..") {
			if (!components.empty()) components.pop_back();
		} else {
			components.push_back(comp);
		}
	}
	return components;
}

std::string	Utils::normalize_path(const std::string& path)
{
	std::vector<std::string> comps = resolve_components(path);
	if (comps.empty()) return "./";

	std::string res = "./";
	for (size_t i = 0; i < comps.size(); ++i) {
		res += comps[i] + (i == comps.size() - 1 ? "" : "/");
	}
	return res;
}

std::string	Utils::normalize_uri(const std::string& uri)
{
	std::vector<std::string> comps = resolve_components(uri);
	if (comps.empty()) return "/";

	std::string res = "";
	for (size_t i = 0; i < comps.size(); ++i) {
		res += "/" + comps[i];
	}

	if (!uri.empty() && uri[uri.length() - 1] == '/' && res != "/") {
		res += "/";
	}
	return res;
}

std::string Utils::sanitizePath(const std::string &path)
{
	if (path.empty()) return "";

	std::string result;

	result.reserve(path.length());
	bool last_was_slash = false;

	for (size_t i = 0; i < path.length(); ++i) {
		if (path[i] == '/') {
			if (!last_was_slash) {
				result += '/';
				last_was_slash = true;
			}
		}
		else {
			result += path[i];
			last_was_slash = false;
		}
	}
	return result;
}

std::string Utils::getMimeType(const std::string& file_path)
{
	size_t dot = file_path.find_last_of(".");
	if (dot == std::string::npos) return "application/octet-stream";

	std::string ext = file_path.substr(dot);
	if (ext == ".html" || ext == ".htm") return "text/html";
	if (ext == ".css") return "text/css";
	if (ext == ".js") return "application/javascript";
	if (ext == ".png") return "image/png";
	if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
	if (ext == ".gif") return "image/gif";
	if (ext == ".ico") return "image/x-icon";
	if (ext == ".txt") return "text/plain";
	if (ext == ".pdf") return "application/pdf";
	return "application/octet-stream";
}

std::string Utils::getStatusMessage(int code)
{
	switch (code) {
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 431: return "Request Header Fields Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 504: return "Gateway Timeout";
		default: return "Unknown Status";
	}
}

bool	Utils::Myaccess(const std::string& path, short& status_code, int mode,
			bool isFile)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		status_code = 404;
		return false;
	}

	if (S_ISDIR(st.st_mode) && isFile) {
		status_code = 403;
		return false;
	}

	if (access(path.c_str(), mode) != 0) {
		if (errno == EACCES)
			status_code = 403;
		else
			status_code = 404;
		return false;
	}

	if (mode & R_OK) {
		if (!(st.st_mode & S_IRUSR) && !(st.st_mode & S_IRGRP)
			&& !(st.st_mode & S_IROTH)) {
			status_code = 403;
			return false;
		}
	}

	if (mode & W_OK) {
		if (!(st.st_mode & S_IWUSR) && !(st.st_mode & S_IWGRP)
			&& !(st.st_mode & S_IWOTH)) {
			status_code = 403;
			return false;
		}
	}

	if (mode & X_OK) {
		if (!(st.st_mode & S_IXUSR) && !(st.st_mode & S_IXGRP)
			&& !(st.st_mode & S_IXOTH)) {
			status_code = 403;
			return false;
		}
	}

	return true;
}
