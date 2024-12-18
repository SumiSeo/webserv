#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <map>
# include <string>
# include <vector>

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

	// -- debugging functions -- //
	void printKeyValues();
};

#endif
