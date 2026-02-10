#pragma once

#include "Directives.hpp"

class	Location
{
	public:
		Location(const Directives& server_directives);
		~Location();

		/* Getters */
		const Directives&			getDirectives() const;
		Directives&					getDirectives();

		/* Setters */
		void						setPath(const std::string& path);
		void						setAlias(const std::string& alias);
		void						logs() const;

	private:
		Directives					_directives;
};
