#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <string>
# include <vector>
# include "Server.hpp"

class WebServer
{
  public:
	WebServer(WebServer const &src);
	WebServer(char const fileName[]);
	~WebServer();

	WebServer &operator=(WebServer const &original);

  protected:
	/* data */
  private:
	typedef std::vector<std::string> t_vecString;
	std::vector<Server> _servers;

	WebServer();
	t_vecString readFile(char const fileName[]);
	void parseTokens(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void searchTokens(t_vecString const &tokens);
	void tokenToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void LocationToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end);


	//debugging functions
	void printKeyValues();
};

#endif
