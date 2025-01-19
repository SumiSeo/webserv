#include "Location.hpp"

using std::map;
using std::string;

Location::Location()
{
}

Location::Location(Location const &src):
	_pairs(src._pairs)
{
}

Location::~Location()
{
}

Location &Location::operator=(Location const &rhs)
{
	_pairs = rhs._pairs;
	return *this;
}

Location::t_vecStrings Location::getValuesOf(string const &target) const
{
	typedef map<string, t_vecStrings> t_mapStringVecStrings;

	t_mapStringVecStrings::const_iterator values = _pairs.find(target);

	if (values == _pairs.end())
		return t_vecStrings();
	
	return values->second;
}

string Location::getValueOf(string const &target) const
{
	t_vecStrings values = getValuesOf(target);

	if (values.empty())
		return string();
	
	return values[0];
}

bool Location::checkValuesOf(std::string const &target, std::string const &testValue) const
{
	t_vecStrings values = getValuesOf(target);

	if (values.empty())
		return false;

	for (t_vecStrings::const_iterator it = values.begin(); it != values.end(); ++it)
	{
		if (*it == testValue)
			return true;
	}

	return false;
}

bool Location::checkValueOf(std::string const &target, std::string const &testValue) const
{
	string value = getValueOf(target);

	return value == testValue;
}
