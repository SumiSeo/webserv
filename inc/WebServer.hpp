#ifndef __SERVER_HPP__
# define __SERVER_HPP__

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

#define BUF_SIZE 500
#define PORT "8080"
#define MAX_EVENTS 100
#define MAX_CLIENTS 10

class WebServer
{
  public:
	WebServer();
	WebServer(const WebServer &src);
	WebServer(const char fileName[]);
	~WebServer();

	WebServer &operator=(const WebServer &original);

  protected:
	/* data */
  private:
	typedef std::vector<std::string> t_vecString;
	std::vector<Server> _servers;

	t_vecString readFile(const char fileName[]);
	void parseTokens(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void searchTokens(const t_vecString &tokens);
	void tokenToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void LocationToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	static void *get_in_addr(struct sockaddr *sa);
	int createServer(void);
	void loop(int socketFd);

	//debugging functions
	void printKeyValues(void);


};

#endif
