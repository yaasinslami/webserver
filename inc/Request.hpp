#ifndef REQUEST_HPP
# define REQUEST_HPP

#include <string>

class	Request
{
	public:
		Request(std::string raw_content);
		~Request();

		std::string getMethod() const;
        std::string getPath() const;
        std::string getVersion() const;

	private:
		std::string	_method;
		std::string	_path;
		std::string	_version;

		void	parse(std::string raw);
};

#endif