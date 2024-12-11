#ifndef Location_HPP
# define Location_HPP

# include <map>
# include <string>
# include <vector>

struct	Location
{
  public:
	std::map<std::string, std::vector<std::string> > pairs;
};

#endif