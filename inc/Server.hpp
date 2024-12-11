#ifndef Server_HPP
# define Server_HPP

# include "Location.hpp"
struct Server
{
   public:
	std::map<std::string, std::vector<std::string> >	_configs;
	std::map<std::string, Location>						_locations;
};

#endif