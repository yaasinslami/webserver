/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-hadr <oel-hadr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/05 00:07:46 by oel-hadr          #+#    #+#             */
/*   Updated: 2026/02/13 15:31:13 by oel-hadr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <vector>
#include <sstream>

#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

using namespace std;

class Utils
{
	public:
		static std::string	normalize_path(const std::string& path);
		static bool			set_nonblocking(int socket);
		static void*		memset(void *p, int c, size_t len);
};
