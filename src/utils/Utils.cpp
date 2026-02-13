/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-hadr <oel-hadr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/05 00:09:01 by oel-hadr          #+#    #+#             */
/*   Updated: 2026/02/13 10:51:54 by oel-hadr         ###   ########.fr       */
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
