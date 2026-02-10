/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-hadr <oel-hadr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/05 00:09:01 by oel-hadr          #+#    #+#             */
/*   Updated: 2025/10/05 00:47:32 by oel-hadr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"

bool Utils::set_nonblocking( int socket )
{
    int flags = fcntl(socket, F_GETFL, 0);

	if (flags == -1)
	{
		cerr << "fcntl F_GETFL" << endl;
		return (false);
	}
	if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		cerr << "fcntl F_SETFL, O_NONBLOCK" << endl;
		return (false);
	}
	return (true);
}

void Utils::printAddr(struct addrinfo* address, const std::string port)
{
	if (address == NULL || address->ai_addr == NULL)
	{
		cerr << "[Unknown] : (no address)\n";
		return;
	}

	char ipBuffer[INET6_ADDRSTRLEN];
	void *addr = NULL;
	const char *version = "Unknown";

	if (address->ai_family == AF_INET)
	{
		struct sockaddr_in *ipv4 = (struct sockaddr_in*) address->ai_addr;
		addr = &(ipv4->sin_addr);
		version = "IPv4";
	}
	else if (address->ai_family == AF_INET6)
	{
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) address->ai_addr;
		addr = &(ipv6->sin6_addr);
		version = "IPv6";
	}
	
	if (addr && inet_ntop(address->ai_family, addr, ipBuffer, sizeof(ipBuffer)) != NULL)
		cout << "[" << version << "] : " << ipBuffer << ":" << port << endl;
	else
		cout << "[" << version << "] : (address unavailable)\n";
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
