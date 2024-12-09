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
}

void Server::parseTokens(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	for (t_vecString::const_iterator it = start; it != end; it++)
	{
		if (*it == "server" || *it == "{")
			continue ;
		t_vecString::const_iterator start = it;
		while (it != end && *it != ";")
			it++;
		tokenToMap(start, it);
		if (it == end)
			break ;
		std::cout << std::endl;
	}
}

void Server::tokenToMap(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	for (t_vecString::const_iterator it = start; it != end; it++)
	{
		std::cout << "YO " << *it << std::endl;
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
