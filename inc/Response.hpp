#ifndef __RESPONSE_HPP__
# define __RESPONSE_HPP__

# include <map>
# include <string>
# include <vector>

# include "Request.hpp"
# include "WebServer.hpp"

class Response
{
  public:
	/* Methods */
	Response();
	Response(Response const &src);
	Response(Request const &request, WebServer::Server const &config);
	~Response();

	Response &operator=(Response const &rhs);

	// Main Methods
	int const &getFdCGI() const;
	std::string &getResponse();
	bool isComplete() const;

  protected:

  private:
	/* Typedefs*/
	typedef std::map<std::string, std::string> t_mapStrings;
	/* Members */
	std::string _buffer;
	int _cgiFd;
	bool _responseComplete;

	/* Methods */
	std::string getFileContent(std::string const &pathname) const;
	char **headersToEnv(t_mapStrings const &headers) const;
};

#endif
