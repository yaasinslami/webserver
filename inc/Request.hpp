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

		class	RequestException : public std::exception
		{
			private:
				int	_status_code;

			public:
				RequestException(int code);
				int get_status_code() const;

				virtual const char * what() const throw();
		};
	private:
		std::string	_method;
		std::string	_path;
		std::string	_version;
		std::string	_host;

		void	parse(std::string raw);

};

#endif