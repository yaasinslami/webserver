#include "../../inc/Directives.hpp"
#include  "../../inc/ConfigParser.hpp"
#include <iostream>

void	Directives::setDirective(const enum e_directive_type directve_type, ConfigParser& conf)
{
	if (directve_type == DIR_ROOT) {
		setRoot(conf.consumeToken());
	}
	else if (directve_type == DIR_INDEX) {
		while (conf.peekToken() != ";") {
			addIndex(conf.consumeToken());
		}
	}
	else if (directve_type == DIR_AUTO_INDEX)
	{
		std::string	autoindex = conf.consumeToken();
		if (autoindex == "on")
			setAutoIndex(true);
		else if (autoindex == "off")
			setAutoIndex(false);
		else
			throw std::runtime_error("Unknown or unexpected in 'autoindex' directive.");
	}
	else if (directve_type == DIR_ALLOWED_METHODS)
	{
		while (conf.peekToken() != ";")
		{
			std::string	method = conf.consumeToken();
			if (method != "GET" && method != "POST" && method != "DELETE")
				throw std::runtime_error("Unknown or Unexpected Method in 'allowed_methods' directive.");
			addMethod(method);
		}
	}
	else if (directve_type == DIR_MAX_BODY_SIZE) {
		setMaxBodySize(std::atol(conf.consumeToken().c_str()));
	}
	else if (directve_type == DIR_ERROR_PAGES) {
		std::vector<int> codes;
		std::string path;

		while (conf.getTokensIndex() < conf.getTokens().size() && std::isdigit(conf.peekToken()[0])) {
			codes.push_back(std::atoi(conf.consumeToken().c_str()));
		}

		if (codes.empty())
			throw std::runtime_error("No error code specified for 'error_page'.");

		if (conf.peekToken() == ";" || conf.peekToken() == "")
			throw std::runtime_error("No path specified for 'error_page'.");

		path = conf.consumeToken();

		for (size_t i = 0; i < codes.size(); ++i) {
			_error_pages[codes[i]] = path;
		}
	}
	else if (directve_type == DIR_UPLOAD) {
		std::string	accept_upload = conf.consumeToken();
		if (accept_upload == "on")
			setUpload(true);
		else if (accept_upload == "off")
			setUpload(false);
		else
			throw std::runtime_error("Unknown or unexpected in 'upload' directive.");
	}
	else if (directve_type == DIR_UPLOAD_LOC) {
		setUploadLoc(conf.consumeToken());
	}
}

Directives::Directives() :
	_root("www"),
	_autoindex(false),
	_client_max_body_size(30000000),
	_upload(true)
{
	_allowed_methods.push_back("GET");
	_allowed_methods.push_back("POST");
	_allowed_methods.push_back("DELETE");
}

Directives::~Directives() {}

Directives::Directives(const Directives& dir) { *this = dir; }

Directives& Directives::operator=(const Directives& dir)
{
	if (this != &dir)
	{
		setRoot(dir.getRoot());
		setIndex(dir.getIndex());
		setAutoIndex(dir.getAutoIndex());
		setAllowedMethods(dir.getAllowedMethods());
		setMaxBodySize(dir.getMaxBodySize());
		setErrorPages(dir.getErrorPages());
	}

	return *this;
}

const std::string&	Directives::getRoot() const { return this->_root; }

const std::vector<std::string>&	Directives::getIndex() const { return this->_index; }

const bool&	Directives::getAutoIndex() const { return this->_autoindex; }

const std::vector<std::string>&	Directives::getAllowedMethods() const { return this->_allowed_methods; }

const unsigned long&	Directives::getMaxBodySize() const { return this->_client_max_body_size; }

const std::map<int, std::string>&	Directives::getErrorPages() const { return this->_error_pages; }

bool	Directives::canUpload() const { return _upload; }

std::string	Directives::getUploadPath() const { return _upload_loc; }

std::string Directives::getPath() const { return _path; }

std::string Directives::getAlias() const { return _alias; }

void	Directives::setRoot(const std::string& root) { _root = root; }

void	Directives::setIndex(const std::vector<std::string>& index) { _index = index; }

void	Directives::addIndex(const std::string& value) { _index.push_back(value); }

void	Directives::setAutoIndex(const bool& autoindex) { this->_autoindex = autoindex; }

void	Directives::setAllowedMethods(const std::vector<std::string>& allowed_methods) { this->_allowed_methods = allowed_methods; }

void	Directives::addMethod(const std::string& method) { this->_allowed_methods.push_back(method); }

void	Directives::setMaxBodySize(const unsigned long& client_max_body_size) { this->_client_max_body_size = client_max_body_size; }

void	Directives::setErrorPages(const std::map<int, std::string>& error_pages) { this->_error_pages = error_pages; }

void	Directives::setUpload(bool accept_upload) { _upload = accept_upload; }

void	Directives::setUploadLoc(const std::string& upload_loc) { _upload_loc = upload_loc; }

bool	Directives::isMethodAllowed(const std::string& method) const { return (std::find(_allowed_methods.begin(), _allowed_methods.end(), method) != _allowed_methods.end()); }

void	Directives::setPath(const std::string& path) { _path = path; }

void	Directives::setAlias(const std::string& alias) { _alias = alias; }

e_directive_type	getDirectiveType(const std::string &directive_str)
{
	static std::map<std::string, e_directive_type> directive_map;

	if (directive_map.empty())
	{
		directive_map["server"] = DIR_SERVER;
		directive_map["location"] = DIR_LOCATION;
		directive_map["root"] = DIR_ROOT;
		directive_map["listen"] = DIR_LISTEN;
		directive_map["index"] = DIR_INDEX;
		directive_map["autoindex"] = DIR_AUTO_INDEX;
		directive_map["alias"] = DIR_ALIAS;
		directive_map["client_max_body_size"] = DIR_MAX_BODY_SIZE;
		directive_map["allowed_methods"] = DIR_ALLOWED_METHODS;
		directive_map["server_name"] = DIR_SERVER_NAME;
		directive_map["error_page"] = DIR_ERROR_PAGES;
		directive_map["upload"] = DIR_UPLOAD;
		directive_map["upload_loc"] = DIR_UPLOAD_LOC;
	}

	std::map<std::string, e_directive_type>::iterator it = directive_map.find(directive_str);

	if (it != directive_map.end())
		return it->second;

	return DIR_UNKNOWN;
}

void	Directives::logs() const
{
	std::cout << "==== Directives ====\n";
	std::cout << "root: \"" << getRoot() << "\"\n";
	// std::cout << "index: \"" << getIndex() << "\"\n";
	std::cout << "index: \"";
	for (size_t i = 0; i < _index.size(); ++i) {
		std::cout << "[" << _index[i] << "]";
		if (i + 1 < _index.size()) std::cout << " ";
	}
	std::cout << "\"\n";

	std::cout << "autoindex: \"" << ((getAutoIndex() == true) ? "on" : "off")  << "\"\n";
	std::cout << "client_max_body_size: \"" << getMaxBodySize() << "\"\n";


	std::cout << "allowed_methods: ";
	for (std::vector<std::string>::const_iterator it = _allowed_methods.begin(); it != _allowed_methods.end(); std::advance(it, 1)) {
		std::cout << "\"" << *it << "\" ";
	}
	std::cout << std::endl;

	std::cout << "error_pages: ";
	if (_error_pages.empty()) {
		std::cout << "(none)" << std::endl;
	} else {
		std::cout << std::endl;
		for (std::map<int, std::string>::const_iterator it = _error_pages.begin(); it != _error_pages.end(); ++it) {
			std::cout << "\t" << it->first << ": \"" << it->second << "\"\n";
		}
	}
}
