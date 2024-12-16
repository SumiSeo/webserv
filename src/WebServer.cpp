#include "WebServer.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecString;

namespace
{
	char const *const	SERVER_KEYS[] = {
		"listen",
		"server_name",
		"error_page",
		NULL,
	};
	char const *const	LOCATION_KEYS[] = {
		"root",
		"autoindex",
		NULL,
	};
}

namespace Utils
{
	t_vecString	split(const string &str, char delim);
	bool		isValidKeyServer(string const &key);
	bool		isValidKeyLocation(string const &key);
};

WebServer::WebServer()
{
	Server server;

}

WebServer::WebServer(const WebServer &src)
{
	if (this != &src)
	{
		*this = src;
	}
}

WebServer::WebServer(const char fileName[])
{
	t_vecString listTokens = readFile(fileName);
	searchTokens(listTokens);
}

WebServer::~WebServer()
{
}

WebServer &WebServer::operator=(const WebServer &rhs)
{
	if (this != &rhs)
	{
		*this = rhs;
	}
	return (*this);
}

// --- Private methods ---

WebServer::t_vecString WebServer::readFile(const char filename[])
{
	t_vecString	listTokens;

	ifstream infile(filename);
	infile.exceptions(ifstream::badbit);
	for (string content; std::getline(infile, content);)
	{
		content = content.substr(0, content.find('#'));
		stringstream stream(content);
		while (stream >> content)
		{
			t_vecString tokens(Utils::split(content, ';'));
			for (t_vecString::const_iterator it(tokens.begin()); it != tokens.end(); ++it)
				listTokens.push_back(*it);
		}
	}
	return (listTokens);
}

void WebServer::searchTokens(const t_vecString &tokens)
{
	int	sizeTokens;
	
	std::string const server = "server";

	sizeTokens = tokens.size();
	for (t_vecString::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		std::string value = *it;
		if (value == server)
		{
			_servers.push_back(Server());
			parseTokens(it + 1, tokens.end());
		}
	}
	printKeyValues();
}

void WebServer::parseTokens(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	if (start == end || *start != "{")
		throw std::runtime_error("Missing token '{'");

	start++;
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

void WebServer::tokenToMap(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	std::string key = *start;
	std::vector<string> names;
	Server &server = _servers.back();

	if (key == "location")	
	{
		 while (start != end) 
		{
		
			++start;
			if (start == end) 
				break; 

			Location location;
			std::vector<std::string> locationValues;
			std::string locationKey = *start;
			++start;
			while (start != end && *start != "}") 
			{
				if (*start == "{") 
				{
					++start; 
					continue;
				}
				std::string key = *start;
				locationValues.clear();
				if (!Utils::isValidKeyLocation(key))
					throw std::runtime_error("Invalid key found in location block");
				++start; 
				while (start != end && *start != ";" && *start != "}") 
				{
					locationValues.push_back(*start);
					++start;
				}
				location._pairs.insert(std::make_pair(key, locationValues));
				if (start != end && *start == ";") 
					++start;
				
			}
			server._locations.insert(std::make_pair(locationKey, location));
    	}
	}
	else
	{
		if (!Utils::isValidKeyServer(key))
			throw std::runtime_error("Invalid key found server block");
		for (t_vecString::const_iterator it = start; it != end;)
		{
			++it;
			if (it != end)
				names.push_back(*it);
		}
		server._configs.insert(std::make_pair(key, names));
	}	
}

void WebServer::printKeyValues(void)
{

	for(std::vector<Server>::const_iterator serverIt = _servers.begin(); serverIt != _servers.end(); serverIt++)
	{
		std::cout << "Server" << std::endl;
		for(std::map<std::string, std::vector<std::string> >::const_iterator it = serverIt->_configs.begin(); it!=serverIt->_configs.end(); it++)
		{
			std::cout << "key [" << it->first << "]" <<std::endl;
			for (std::vector<std::string>::const_iterator valIt = it->second.begin(); valIt != it->second.end(); ++valIt)
				std::cout << "  value [" << *valIt << "]"<< std::endl;
		}	

		for (std::map<std::string, Location>::const_iterator it = serverIt->_locations.begin(); it != serverIt->_locations.end(); ++it) 
		{
			std::cout << "Location : key [" << it->first << "]" << std::endl;
			const Location& loc = it->second;
			for (std::map<std::string, std::vector<std::string> >::const_iterator pairIt = loc._pairs.begin(); 
				pairIt != loc._pairs.end(); 
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
}
//---Static functions-- -

t_vecString Utils::split(const string &str, char delim)
{
	t_vecString	words;

	std::size_t pos = 0;
	while (true)
	{
		std::size_t end = str.find(delim, pos);
		if (end == string::npos)
			break ;
		string	token = str.substr(pos, end - pos);
		if (!token.empty())
			words.push_back(token);
		words.push_back(str.substr(end, 1)); // add the delim
		pos = end + 1;
	}
	if (pos < str.size())
		words.push_back(str.substr(pos));
	return (words);
}

bool	Utils::isValidKeyServer(string const &key)
{
	for (std::size_t i = 0; SERVER_KEYS[i] != NULL; ++i)
	{
		if (key == SERVER_KEYS[i])
			return true;
	}
	return false;
}

bool	Utils::isValidKeyLocation(string const &key)
{
	for (std::size_t i = 0; LOCATION_KEYS[i] != NULL; ++i)
	{
		if (key == LOCATION_KEYS[i])
			return true;
	}
	return false;
}
