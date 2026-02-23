/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-hadr <oel-hadr@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/05 00:07:46 by oel-hadr          #+#    #+#             */
/*   Updated: 2026/02/20 21:07:25 by oel-hadr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <fcntl.h>

using namespace std;

class Utils
{
	public:
		static std::string	normalize_path(const std::string& path);
		static std::string normalize_uri(const std::string&);
		static bool			set_nonblocking(int socket);
		static void*		memset(void *p, int c, size_t len);
		static std::string urlDecode(const std::string&);
		static std::string getMimeType(const std::string&);

		template <typename T>
		static std::string	toString(const T& value)
		{
			std::stringstream ss;
			ss << value;
			return ss.str();
		}
};
