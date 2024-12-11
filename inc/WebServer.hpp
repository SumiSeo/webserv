#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <string>
# include <vector>
# include "Server.hpp"

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


	//debugging functions
	void printKeyValues(void);
};

#endif
