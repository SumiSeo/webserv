#ifndef __WEBSERVER_HPP__
# define __WEBSERVER_HPP__

# include <map>
# include <set>
# include <string>
# include <vector>

# include "Server.hpp"

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

class Request;

class WebServer
{
  public:
	/* Typedefs */
	typedef std::vector<std::string> t_vecString;

	/* Methods */
	WebServer(char const fileName[]);
	~WebServer();

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
	std::map<int, int> _cgiFdToResponseFd;

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
	bool handleCGIInput(int fd);

	// -- debugging functions -- //
	void printKeyValues();
};

#endif
