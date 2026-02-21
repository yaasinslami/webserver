/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-hadr <oel-hadr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/05 00:09:01 by oel-hadr          #+#    #+#             */
/*   Updated: 2026/02/21 01:20:09 by oel-hadr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

std::string Utils::normalize_path(const std::string& path)
{
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string component;

    while (std::getline(ss, component, '/'))
    {
        if (component == "" || component == ".")
            continue;
        if (component == "..")
        {
            if (!components.empty())
                components.pop_back();
        }
        else
        {
            components.push_back(component);
        }
    }

    std::string result = "./";
    for (size_t i = 0; i < components.size(); ++i)
    {
        result += components[i];
        if (i < components.size() - 1)
            result += "/";
    }

    // Handle the edge case where the path was just "/" or empty
    if (components.empty())
        return "./";

    return result;
}