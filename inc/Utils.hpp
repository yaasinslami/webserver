/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-hadr <oel-hadr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/05 00:07:46 by oel-hadr          #+#    #+#             */
/*   Updated: 2026/02/13 10:52:03 by oel-hadr         ###   ########.fr       */
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
		static void*	memset(void *p, int c, size_t len);
};
