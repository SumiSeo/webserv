#ifndef __LOCATION_HPP__
# define __LOCATION_HPP__

# include <map>
# include <string>
# include <vector>

struct	Location
{
	/* Typedefs */
	typedef std::vector<std::string> t_vecStrings;

	/* Members */
	std::map<std::string, t_vecStrings> _pairs;

	/* Methods */
	Location();
	Location(Location const &src);
	~Location();

	Location &operator=(Location const &rhs);

	t_vecStrings getValuesOf(std::string const &target) const;
	std::string getValueOf(std::string const &target) const;
	bool checkValuesOf(std::string const &target, std::string const &testValue) const;
	bool checkValueOf(std::string const &target, std::string const &testValue) const;
};

#endif
