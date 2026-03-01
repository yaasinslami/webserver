#include "Directives.hpp"
#include  "ConfigParser.hpp"
#include <iostream>
#include <algorithm>

void	Directives::setDirective(const enum e_directive_type directve_type, ConfigParser& conf)
{
	if (directve_type == DIR_ROOT) {
		setRoot(conf.consumeToken());
		if (getRoot()[getRoot().length() - 1] != '/')
			setRoot(getRoot() + "/");
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
		setMaxBodySize( Directives::parseSizeString(conf.consumeToken()));
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
	else if (directve_type == DIR_CGI_HANDLER) {
		std::string extension = conf.consumeToken();
		std::string handler_path = conf.consumeToken();
		if (extension.empty() || handler_path.empty())
			throw std::runtime_error("Invalid 'cgi_pass' directive. Expected: cgi_pass <extension> <handler_path>;");
		_cgi_handlers[extension] = handler_path;
	}
}

size_t	Directives::parseSizeString(const std::string& size_str)
{
	const char* str = size_str.c_str();
	char *endptr;
	errno = 0;

	unsigned long len = std::strtoul(str, &endptr, 10);

	if (errno == ERANGE || endptr == str) {
		throw std::runtime_error("Config Error: Invalid size value: " + size_str);
	}

	size_t multiplier = 1;

	if (*endptr != '\0') {
		if (std::strlen(endptr) > 1) throw std::runtime_error("Config Error: Invalid suffix in size value: " + size_str);
		
		char suffix = std::toupper(*endptr);
		if (suffix == 'K') multiplier = 1024;
		else if (suffix == 'M') multiplier = 1024 * 1024;
		else if (suffix == 'G') multiplier = 1024 * 1024 * 1024;
		else throw std::runtime_error("Config Error: Unknown suffix in size value: " + size_str);
	}
	
	size_t parsed_size;
	if (len > ABSOLUTE_MAX_BODY_SIZE / multiplier) {
		std::cerr << "[WARNING] Config size (" << size_str 
				<< ") exceeds safe limit! Capping at absolute max.\n";
		parsed_size = ABSOLUTE_MAX_BODY_SIZE;
	}
	else parsed_size = static_cast<size_t>(len) * multiplier;

	return parsed_size;
}

Directives::Directives() :
	_root("www"),
	_autoindex(false),
	_method_mask(M_GET | M_POST | M_DELETE),
	_methods_explicitly_set(false),
	_client_max_body_size(parseSizeString("100M")),
	_upload(false)
{}

Directives::~Directives() {}

Directives::Directives(const Directives& dir) { *this = dir; }

Directives& Directives::operator=(const Directives& dir)
{
	if (this != &dir)
	{
		setRoot(dir.getRoot());
		setAlias(dir.getAlias());
		setPath(dir.getPath());
		setIndex(dir.getIndex());
		setAutoIndex(dir.getAutoIndex());
		_method_mask = dir._method_mask;
		_methods_explicitly_set = dir._methods_explicitly_set;
		setMaxBodySize(dir.getMaxBodySize());
		setErrorPages(dir.getErrorPages());
		setUpload(dir.canUpload());
		setUploadLoc(dir.getUploadPath());

		_cgi_handlers = dir.getCgiHandlers();
	}

	return *this;
}

const std::string&	Directives::getRoot() const { return this->_root; }

const std::vector<std::string>&	Directives::getIndex() const { return this->_index; }

const bool&	Directives::getAutoIndex() const { return this->_autoindex; }

const size_t&	Directives::getMaxBodySize() const { return this->_client_max_body_size; }

const std::map<int, std::string>&	Directives::getErrorPages() const { return this->_error_pages; }

bool	Directives::canUpload() const { return _upload; }

std::string	Directives::getUploadPath() const { return _upload_loc; }

std::string Directives::getPath() const { return _path; }

std::string Directives::getAlias() const { return _alias; }

void	Directives::setRoot(const std::string& root) { _root = root; }

void	Directives::setIndex(const std::vector<std::string>& index) { _index = index; }

void	Directives::addIndex(const std::string& value) { _index.push_back(value); }

void	Directives::setAutoIndex(const bool& autoindex) { this->_autoindex = autoindex; }

void	Directives::addMethod(const std::string& method)
{
	if (!_methods_explicitly_set) {
		_method_mask = M_NONE;
		_methods_explicitly_set = true;
	}

	if (method == "GET")         _method_mask |= M_GET;
	else if (method == "POST")   _method_mask |= M_POST;
	else if (method == "DELETE") _method_mask |= M_DELETE;
}

void	Directives::setMaxBodySize(const size_t& client_max_body_size)
{
	_client_max_body_size = client_max_body_size;
}

void	Directives::setErrorPages(const std::map<int, std::string>& error_pages) { this->_error_pages = error_pages; }

void	Directives::setUpload(bool accept_upload) { _upload = accept_upload; }

void	Directives::setUploadLoc(const std::string& upload_loc) { _upload_loc = upload_loc; }

bool	Directives::isMethodAllowed(Method m) const { return (_method_mask & m); }

void	Directives::setPath(const std::string& path) { _path = path; }

void	Directives::setAlias(const std::string& alias) { _alias = alias; }

const std::map<std::string, std::string>& Directives::getCgiHandlers() const { return _cgi_handlers; }

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
		directive_map["cgi_pass"] = DIR_CGI_HANDLER;
	}

	std::map<std::string, e_directive_type>::iterator it = directive_map.find(directive_str);

	if (it != directive_map.end())
		return it->second;

	return DIR_UNKNOWN;
}

Method Directives::stringToMethod(const std::string& method_str)
{
	if (method_str == "GET") return M_GET;
	if (method_str == "POST") return M_POST;
	if (method_str == "DELETE") return M_DELETE;
	return M_NONE;
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
	if (_methods_explicitly_set) {
		std::cout << "\"";
		if (isMethodAllowed(M_GET)) std::cout << "GET ";
		if (isMethodAllowed(M_POST)) std::cout << "POST ";
		if (isMethodAllowed(M_DELETE)) std::cout << "DELETE ";
		std::cout << "\"\n";
	} else {
		std::cout << "(default: GET POST DELETE)\n";
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
