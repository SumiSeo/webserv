#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <map>
# include <set>
# include <string>
# include <vector>

# include "Request.hpp"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>

class Response;

#define BUF_SIZE 500
#define MAX_CLIENTS 10

class WebServer
{
  public:
	/* Methods */
	WebServer(char const fileName[]);
	~WebServer();

	// Typedefs
	typedef std::vector<std::string> t_vecString;

	/* New Variable Types */
	struct	Location
	{
		std::map<std::string, t_vecString> _pairs;
	};
	struct Server
	{
		std::map<std::string, t_vecString> _configs;
		std::map<std::string, Location> _locations;
	};
	void loop();

  protected:
	/* data */
  private:
	/* Members */
	std::vector<Server> _servers;
	std::set<int> _socketFds;
	int _epollFd;
	std::map<int, Request> _requests;
	std::map<int, Response> _responses;

	/* Methods */
	WebServer();
	WebServer(WebServer const &src);

	WebServer &operator=(WebServer const &original);

	// -- Main functions -- //
	t_vecString readFile(char const fileName[]);
	void searchTokens(t_vecString const &tokens);

	// -- Utils functions -- //
	void parseTokens(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void tokenToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void LocationToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	static void *get_in_addr(struct sockaddr *sa);
	int createServer();

	// -- debugging functions -- //
	void printKeyValues();
};

#endif
