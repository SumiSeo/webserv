#include "Server.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecString;

namespace Utils
{
t_vecString	split(const string &str, char delim);
};

Server::Server()
{
}

Server::Server(const Server &src)
{
	if (this != &src)
	{
		*this = src;
	}
}

Server::Server(const char fileName[])
{
	t_vecString listTokens(readFile(fileName));
	searchTokens(listTokens);
}

Server::~Server()
{
}

Server &Server::operator=(const Server &rhs)
{
	if (this != &rhs)
	{
		*this = rhs;
	}
	return (*this);
}

// --- Private methods ---

Server::t_vecString Server::readFile(const char filename[])
{
	t_vecString	listTokens;

	ifstream infile(filename);
	infile.exceptions(ifstream::badbit);
	for (string content; std::getline(infile, content);)
	{
		stringstream stream(content);
		while (stream >> content)
		{
			if (content[0] == '#')
				break ;
			t_vecString tokens(Utils::split(content, ';'));
			for (t_vecString::const_iterator it(tokens.begin()); it != tokens.end(); ++it)
				listTokens.push_back(*it);
		}
	}
	return (listTokens);
}

void Server::searchTokens(const t_vecString &tokens)
{
	int	sizeTokens;

	std::string value;
	std::string server = "server";
	sizeTokens = tokens.size();
	for (t_vecString::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		value = *it;
		if (value == server)
		{
			parseTokens(it, tokens.end());
			break ;
		}
	}
	printKeyValues();
}

void Server::parseTokens(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	int depthCheck = 1;
	for (t_vecString::const_iterator it = start; it != end; it++)
	{
		if (*it == "server" || *it == "{" || *it == "}")
			continue ;
		t_vecString::const_iterator restart = it;
		while (it != end)
		{
			if (*it == "{") 
			{
				++it;
				depthCheck = 0;
			} 
			else if (*it == ";" && depthCheck == 1) {
        		tokenToMap(restart, it);
        		restart = ++it; 
			} 
			else if (*it == "}") 
			{
				tokenToMap(restart, it);
				depthCheck = 1;
				break; 
			} 
			else 
				it++;
		}
		if (it == end)
			break ;
	}
}

void Server::tokenToMap(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	std::string key = *start;
	std::vector<string> names;
	if (key == "location")	
		LocationToMap(start, end);
	else
	{
		for (t_vecString::const_iterator it = start; it != end;)
		{
			++it;
			if (it != end)
				names.push_back(*it);
		}
		_configs.insert(std::make_pair(key, names));
	}	

}

void Server::LocationToMap(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	Location location;
	std::vector<string> locationValues;
	start++;
	std::string locationKey = *start;	
	for (t_vecString::const_iterator it = start; it != end;)
	{
		++it;
		if(*it== "{")
			continue;
		if (it != end)
		{
			std::string key = *it;
			it++;
			if(*it!=";" && it!=end)
				locationValues.push_back(*it);
			location.pairs.insert(std::make_pair(key, locationValues));
		}
		 _locations.insert(std::make_pair(locationKey,location));
	}
}

//---Static functions-- -

t_vecString Utils::split(const string &str, char delim)
{
	t_vecString	words;

	std::size_t pos(0);
	while (true)
	{
		std::size_t end = str.find(delim, pos);
		if (end == string::npos)
			break ;
		words.push_back(str.substr(pos, end - pos));
		words.push_back(str.substr(end, 1)); // add the delim
		pos = end + 1;
	}
	if (pos < str.size())
		words.push_back(str.substr(pos));
	return (words);
}

void Server::printKeyValues(void)
{
	for(std::map<std::string, std::vector<std::string> >::iterator it = _configs.begin(); it!=_configs.end(); it++)
	{
		std::cout << "key [" << it->first << "]" <<std::endl;
	    for (std::vector<std::string>::iterator valIt = it->second.begin(); valIt != it->second.end(); ++valIt)
        	std::cout << "  value [" << *valIt << "]"<< std::endl;
	}	

	for (std::map<std::string, Location>::iterator it = _locations.begin(); it != _locations.end(); ++it) 
	{
    	std::cout << "key [" << it->first << "]" << std::endl;
    	const Location& loc = it->second;
		for (std::map<std::string, std::vector<std::string> >::const_iterator pairIt = loc.pairs.begin(); 
			pairIt != loc.pairs.end(); 
			++pairIt) 
		{
			std::cout << "  pair key [" << pairIt->first << "]"<< std::endl;
			for (std::vector<std::string>::const_iterator valIt = pairIt->second.begin(); 
				valIt != pairIt->second.end(); 
				++valIt)
				std::cout << "  pair value [" << *valIt << "]"<< std::endl;
		}
	}
}