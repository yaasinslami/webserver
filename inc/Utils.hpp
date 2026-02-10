/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-hadr <oel-hadr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/05 00:07:46 by oel-hadr          #+#    #+#             */
/*   Updated: 2025/10/05 00:47:19 by oel-hadr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>

#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

using namespace std;

class Utils
{
	public:
		static bool		set_nonblocking(int socket);
		static void		printAddr(struct addrinfo* address, const std::string port);
		static void*	memset(void *p, int c, size_t len);
};
