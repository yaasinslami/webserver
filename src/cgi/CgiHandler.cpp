#include "CgiHandler.hpp"
#include <sys/epoll.h>

static string	mytoupper(std::string str) 
{
	for (size_t i = 0; i < str.length(); i++)
	{
		if (str[i] == '-')
			str[i] = '_';
		else
			str[i] = toupper(str[i]);
	}
	return str;
}

char	**CgiHandler::create_env( Client& client, const std::string& script_path)
{
	Request& req = client.getRequest();
	const Directives* config = client.getConfig();

	std::vector<std::string> env_vars;

	string path = req.getPath();

	string query_string = "";
	size_t q_pos = path.find('?');
	if (q_pos != string::npos)
	{
		query_string = path.substr(q_pos + 1);
		path = path.substr(0, q_pos);
	}

	env_vars.push_back("REQUEST_METHOD=" + req.getMethod());
	env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env_vars.push_back("SCRIPT_FILENAME=" + script_path);
	env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_vars.push_back("SCRIPT_NAME=" + path);
	env_vars.push_back("SCRIPT_FILENAME=" + script_path);
	env_vars.push_back("REQUEST_URI=" + req.getPath());
	env_vars.push_back("DOCUMENT_ROOT=" + config->getRoot());
	env_vars.push_back("QUERY_STRING=" + query_string);
	env_vars.push_back("REDIRECT_STATUS=200");

	try
	{
		std::stringstream ss;
		ss << req.getBodySize();
		env_vars.push_back("CONTENT_LENGTH=" + ss.str());
		env_vars.push_back("CONTENT_TYPE=" + req.getHeader("content-type"));
	}
	catch ( const exception& e )
	{
		std::cout << "CONTENT_LENGTH or CONTENT_TYPE is missing" << endl;
	}

	const map<string, string> headers = req.getHeaders();
	for (map<string, string>::const_iterator it = headers.begin();
		 it != headers.end(); ++it)
	{
		string env_header = "HTTP_" + it->first;
		string envVar = env_header + "=" + it->second;
		env_vars.push_back(mytoupper(envVar));
	}

	char** envp = new char*[env_vars.size() + 1];
	for (size_t i = 0; i < env_vars.size(); ++i)
	{
		envp[i] = new char[env_vars[i].length() + 1];
		strcpy(envp[i], env_vars[i].c_str());
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
	cerr << "CGI Child Error: " << msg << endl;
	exit(1);
}

void CgiHandler::execute(Client* client, const string& script_path,
		const string& interpreter, EpollManager *poller)
{
	const Request&      req = client->getRequest();
	const Directives* config = client->getConfig();

	int cgi_out_pipe[2] = {-1, -1};

	if (pipe(cgi_out_pipe) == -1)
	{
		cerr << "Pipe error" << endl;
		client->setStatus(500);
		return;
	}

	char** envp = create_env(*client, script_path);
	string script_directory = script_path.substr(0, script_path.rfind('/'));

	pid_t pid = fork();
	if (pid == -1)
	{
		cerr << "Fork error" << endl;
		delete_env(envp);
		client->setStatus(500);
		close(cgi_out_pipe[READ_END]);
		close(cgi_out_pipe[WRITE_END]);
		return;
	}

	if (pid == 0)
	{
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

		const char* script_path_arg = script_path.c_str();
		char* argv[] = {
			const_cast<char*>(interpreter.c_str()),
			const_cast<char*>(script_path_arg),
			NULL
		};

		execve(interpreter.c_str(), argv, envp);
		child_exit_error("execve failed");
	}

	close(cgi_out_pipe[WRITE_END]);

	Utils::set_nonblocking(cgi_out_pipe[READ_END]);

	client->setCgiPid(pid);
	client->setCgiOutPipe(cgi_out_pipe[READ_END]);

	client->setState(HANDLING_CGI);
	delete_env(envp);

	if (!poller->add_cgi_fds(client))
		cout << "[poller]: register_cgi_fds failed" << endl;
}

std::string	CgiHandler::prepare(Client *client, EpollManager& poller, int fd)
{
	const Directives* config = client->getConfig();
	Request& req = client->getRequest();
	string	path = req.getPath();
	size_t	q_pos = path.find('?');

	if (q_pos != string::npos)
		path = path.substr(0, q_pos);

	string script_fs_path = Utils::normalize_path(config->getRoot() + path);

	if (access(script_fs_path.c_str(), F_OK) == -1)
	{
		client->setStatus(400);
		std::string body = "<html>\r\n"
						"<head><title>400 Bad Request</title></head>\r\n"
						"<body>\r\n"
						"<center><h1>400 Bad Request</h1></center>\r\n"
						"<hr><center>webserv</center>\r\n"
						"</body>\r\n"
						"</html>\r\n";

		std::string response = "HTTP/1.1 400 Bad Request\r\n"
							"Content-Type: text/html\r\n"
							"Content-Length: " + Utils::toString(body.length()) + "\r\n"
							"Connection: close\r\n\r\n" + 
							body;

		client->set_write_buf(response);
		client->markWriting();
		poller.set_events(fd, EPOLLOUT);
		client->getRequest().setIsCgi(false);
		return "";
	}

	return (script_fs_path);
}
