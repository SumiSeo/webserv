#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <map>
# include <string>
# include <vector>

# include "Location.hpp"

struct Server
{
	/* Typedefs */
	typedef std::vector<std::string> t_vecStrings;

	/* Members */
	std::map<std::string, t_vecStrings> _configs;
	std::map<std::string, Location> _locations;
	std::map<std::string, std::string> _errorPages;

	/* Methods */
	Server();
	Server(Server const &src);
	~Server();

	Server &operator=(Server const &rhs);

	t_vecStrings getValuesOf(std::string const &target) const;
	std::string getValueOf(std::string const &target) const;
	std::string searchLocationKey(std::string requestTarget) const;
	bool checkValuesOf(std::string const &target, std::string const &testValue) const;
	bool checkValueOf(std::string const &target, std::string const &testValue) const;
	std::string getErrorPage(std::string const &errorCode);
	std::string getErrorPage(int errorCode);
};

#endif
