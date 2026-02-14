#pragma once

#include <string>

#include "Client.hpp"
#include "EpollManager.hpp"

#define READ_END 0
#define WRITE_END 1

class EpollManager;

class CgiHandler
{
    private:

    public:
        static void execute(Client* client, const string& script_path, const string& interpreter, EpollManager* poller);

    private:
    static char** create_env(Client& client, const std::string& script_path);
    static char** create_args(const string& script, const string& interpreter);
    static void   delete_env(char **envp);
};
