#include "CgiHandler.hpp"
#include <sys/epoll.h>

char	**CgiHandler::create_env( Client& client, const std::string& script_path)
{
	Request& req = client.getRequest();
	const Directives* config = client.getConfig();
	std::vector<std::string> env_vars;

	env_vars.push_back("REQUEST_METHOD=" + req.getMethod());
	env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env_vars.push_back("SCRIPT_FILENAME=" + Utils::resolve_path(script_path));
	env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_vars.push_back("REQUEST_URI=" + req.getPath());
	env_vars.push_back("DOCUMENT_ROOT=" + config->getRoot());
	env_vars.push_back("QUERY_STRING=" + req.getQuery());
	env_vars.push_back("REDIRECT_STATUS=200");

	if (req.getMethod() == "POST" && config->canUpload())
		env_vars.push_back("UPLOAD_DIR=" + Utils::resolve_path(Utils::normalize_path(config->getRoot() + config->getUploadPath())));

	try
	{
		std::stringstream ss;
		ss << req.getBodySize();
		env_vars.push_back("CONTENT_LENGTH=" + ss.str());
		env_vars.push_back("CONTENT_TYPE=" + req.getHeader("content-type"));
	}
	catch ( const std::exception& e )
	{
	}

	const std::map<std::string, std::string> headers = req.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		std::string key = it->first;
		for (size_t i = 0; i < key.length(); ++i) {
			if (key[i] == '-') key[i] = '_';
			key[i] = toupper(key[i]);
		}
		env_vars.push_back("HTTP_" + key + "=" + it->second);
	}

	char** envp = new char*[env_vars.size() + 1];
	for (size_t i = 0; i < env_vars.size(); ++i)
	{
		envp[i] = new char[env_vars[i].length() + 1];
		std::strcpy(envp[i], env_vars[i].c_str());
	}

	envp[env_vars.size()] = NULL;

	return envp;
}

void	CgiHandler::delete_env(char** envp)
{
	if (!envp)
		return ;
	for (int i = 0; envp[i] != NULL; ++i)
		delete[] envp[i];
	delete[] envp;
}

static void	child_exit_error(const char* msg)
{
	std::cerr << "CGI Child Error: " << msg << std::endl;
	std::exit(1);
}

void CgiHandler::execute(Client* client, const std::string& script_path,
		const std::string& interpreter, EpollManager &poller)
{
	const Request&      req = client->getRequest();

	int cgi_out_pipe[2] = {-1, -1};

	if (pipe(cgi_out_pipe) == -1)
	{
		std::cerr << "Pipe error" << std::endl;
		client->setStatus(500);
		client->getResponse().generateError();
		return;
	}

	char** envp = create_env(*client, script_path);

	pid_t pid = fork();
	if (pid == -1)
	{
		std::cerr << "Fork error" << std::endl;
		delete_env(envp);
		client->setStatus(500);
		client->getResponse().generateError();
		close(cgi_out_pipe[READ_END]);
		close(cgi_out_pipe[WRITE_END]);
		return;
	}

	if (pid == 0)
	{
		size_t last_slash = script_path.rfind('/');
		std::string script_directory = (last_slash == std::string::npos) ? "." : script_path.substr(0, last_slash);
		std::string script_filename = (last_slash == std::string::npos) ? script_path : script_path.substr(last_slash + 1);

		close(cgi_out_pipe[READ_END]);

		int body_fd = -1;

		if (!req.getBodyPath().empty()) {
			body_fd = open(req.getBodyPath().c_str(), O_RDONLY);
		}

		if (body_fd == -1) {
			body_fd = open("/dev/null", O_RDONLY);
		}

		if (dup2(body_fd, STDIN_FILENO) == -1)
			child_exit_error("dup2 stdin failed");
		
		close(body_fd);

		if (dup2(cgi_out_pipe[WRITE_END], STDOUT_FILENO) == -1)
			child_exit_error("dup2 stdout failed");

		close(cgi_out_pipe[WRITE_END]);

		if (chdir(script_directory.c_str()) == -1)
			child_exit_error("chdir failed");

		char* argv[] = {
			const_cast<char*>(interpreter.c_str()),
			const_cast<char*>(script_filename.c_str()),
			NULL
		};

		execve(interpreter.c_str(), argv, envp);
		child_exit_error("execve failed");
	}

	close(cgi_out_pipe[WRITE_END]);

	Utils::set_nonblocking(cgi_out_pipe[READ_END]);

	client->setCgiPid(pid);
	client->setCgiOutPipe(cgi_out_pipe[READ_END]);
	client->setCgiStartTime(time(NULL));

	client->setState(HANDLING_CGI);
	delete_env(envp);

	if (!poller.add_cgi_fds(client))
		std::cout << "[poller]: register_cgi_fds failed" << std::endl;
}

std::string	CgiHandler::prepare(Client *client)
{
	const Directives* config = client->getConfig();
	Request& req = client->getRequest();

	std::string script_fs_path = Utils::normalize_path(config->getRoot() + req.getPath());
	
	if (!Utils::Myaccess(script_fs_path, client->getStatus(), R_OK | X_OK, true))
		return "";

	return script_fs_path;
}
