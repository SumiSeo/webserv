#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <map>
# include <string>
# include <vector>

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

#define BUF_SIZE 500
#define PORT "8080"
#define MAX_EVENTS 100
#define MAX_CLIENTS 10

class WebServer
{
  public:
	/* Members */
	WebServer(WebServer const &src);
	WebServer(char const fileName[]);
	~WebServer();

	WebServer &operator=(WebServer const &original);

  protected:
	/* data */
  private:
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

	/* Members */
	std::vector<Server> _servers;
	std::vector<int> _socketFds;

	/* Methods */
	WebServer();

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
	int createServer(void);
	void loop(int socketFd);

	// -- debugging functions -- //
	void printKeyValues();
};

#endif
