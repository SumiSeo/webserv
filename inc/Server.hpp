#ifndef __SERVER_HPP__
# define __SERVER_HPP__

# include <string>
# include <vector>

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
	typedef std::vector<std::string>	t_vecString;

	t_vecString	readFile(const char fileName[]);
	void		parseTokens(const t_vecString &tokens);
};

#endif
