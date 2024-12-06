#include "Server.hpp"
#include <fstream>
#include <iostream>
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
	parseTokens(listTokens);
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

void Server::parseTokens(const t_vecString &tokens)
{
	int	sizeTokens;

	sizeTokens = tokens.size();
	for (t_vecString::const_iterator it = tokens.begin(); it != tokens.end(); ++it)
		std::cout << *it << std::endl;
}

// --- Static functions ---

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
