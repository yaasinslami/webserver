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

		static bool						compareLocations(const Location& a, const Location& b);

	private:
		Directives					_directives;
};
