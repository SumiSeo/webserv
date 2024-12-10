#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <string>
# include <vector>
# include "Location.hpp"

class Server
{
  public:
	Server();
	Server(const Server &src);
	Server(const char fileName[]);
	~Server();

	Server &operator=(const Server &original);

  protected:
	/* data */
  private:
	typedef std::vector<std::string> t_vecString;

	std::map<std::string, std::vector<std::string> >	_configs;
	std::map<std::string, Location>						_locations;

	t_vecString readFile(const char fileName[]);
	void parseTokens(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void searchTokens(const t_vecString &tokens);
	void tokenToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end);
	void LocationToMap(t_vecString::const_iterator start,
		t_vecString::const_iterator end, std::string name);
};

#endif
