#include <cstring>
#include <sstream>

#include "Server.hpp"

using std::map;
using std::string;
using std::stringstream;

namespace
{
	std::size_t lenLongestPrefix(char const str1[], char const str2[]);
}

Server::Server()
{
}

Server::Server(Server const &src):
	_configs(src._configs),
	_locations(src._locations)
{
}

Server::~Server()
{
}

Server &Server::operator=(Server const &rhs)
{
	_configs = rhs._configs;
	_locations = rhs._locations;
	return *this;
}

Server::t_vecStrings Server::getValuesOf(string const &target) const
{
	typedef map<string, t_vecStrings> t_mapStringVecStrings;

	t_mapStringVecStrings::const_iterator values = _configs.find(target);

	if (values == _configs.end())
		return t_vecStrings();

	return values->second;
}

string Server::getValueOf(string const &target) const
{
	t_vecStrings values = getValuesOf(target);

	if (values.empty())
		return string();

	return values[0];
}

string Server::searchLocationKey(string const &requestTarget) const
{
	typedef map<string, Location> t_mapStringLocs;

	t_mapStringLocs::const_iterator locationBlock;
	std::size_t lenMax = 0;

	for (t_mapStringLocs::const_iterator it = _locations.begin(); it != _locations.end(); ++it)
	{
		string const &locationKey = it->first;
		std::size_t lenPrefix = 0;

		if (locationKey[locationKey.size() - 1] == '/')
			lenPrefix = lenLongestPrefix(locationKey.c_str(), requestTarget.c_str());
		else if (std::strcmp(locationKey.c_str(), requestTarget.c_str()) == 0)
			return locationKey;

		if (lenPrefix > lenMax)
		{
			locationBlock = it;
			lenMax = lenPrefix;
		}
	}

	if (lenMax != 0)
		return locationBlock->first;
	return string("");
}

bool Server::checkValuesOf(std::string const &target, std::string const &testValue) const
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

bool Server::checkValueOf(std::string const &target, std::string const &testValue) const
{
	string value = getValueOf(target);

	return value == testValue;
}

string Server::getErrorPage(string const &errorCode)
{
	typedef map<string, string> t_mapStrings;

	t_mapStrings::const_iterator errorPage = _errorPages.find(errorCode);
	if (errorPage == _errorPages.end())
		return string();
	return errorPage->second;
}

string Server::getErrorPage(int errorCode)
{
	stringstream ss;
	ss << errorCode;
	return getErrorPage(ss.str());
}

namespace
{
	std::size_t lenLongestPrefix(char const str1[], char const str2[])
	{
		std::size_t i = 0;
		while (str1[i] != '\0' && str2[i] != '\0')
		{
			if (str1[i] != str2[i])
				break;
			++i;
		}
		return i;
	}
}
