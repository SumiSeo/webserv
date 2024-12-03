#include "Server.hpp"

Server::Server() { }

Server::Server(const Server &src) { }

Server::Server(const char fileName[])
{
	t_vecString	listTokens(readFile(fileName));

	parseTokens(listTokens);
}

Server::~Server() { }

Server	&Server::operator=(const Server &rhs)
{

	return *this;
}

// --- Private methods ---

Server::t_vecString	Server::readFile(const char filename[])
{
}

void	Server::parseTokens(const t_vecString &tokens)
{
}
