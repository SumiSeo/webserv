#include <cstring>
#include <vector>
#include <sstream>

#include "Server.hpp"

using std::map;
using std::string;
using std::vector;

typedef vector<string> t_vecStrings;
using std::stringstream;

namespace
{
	std::size_t lenLongestPrefix(string const &str1, string const &str2);
	t_vecStrings split(string const &str, char sep);
}

Server::Server()
{
}

Server::Server(Server const &src):
	_configs(src._configs),
	_locations(src._locations),
	_errorPages(src._errorPages)
{
}

Server::~Server()
{
}

Server &Server::operator=(Server const &rhs)
{
	_configs = rhs._configs;
	_locations = rhs._locations;
	_errorPages = rhs._errorPages;
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
			lenPrefix = lenLongestPrefix(locationKey, requestTarget);
		else if (locationKey.size() == requestTarget.size()
			&& std::strcmp(locationKey.c_str(), requestTarget.c_str()) == 0)
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
	std::size_t lenLongestPrefix(string const &str1, string const &str2)
	{
		t_vecStrings components1 = split(str1, '/');
		t_vecStrings components2 = split(str2, '/');
		std::size_t i = 0;
		for (t_vecStrings::const_iterator it1 = components1.begin(), it2 = components2.begin(); it1 != components1.end() && it2 != components2.end(); ++it1, ++it2)
		{
			if (*it1 != *it2)
				break;
			++i;
		}
		return i;
	}
	t_vecStrings split(string const &str, char sep)
	{
		t_vecStrings output;
		std::size_t pos = 0;
		while (true)
		{
			std::size_t end = str.find(sep, pos);
			if (end == string::npos)
				break;
			output.push_back(str.substr(pos, end - pos + 1));
			pos = end + 1;
		}
		string temp = str.substr(pos);
		if (!temp.empty())
			output.push_back(temp);
		return output;
	}
}
