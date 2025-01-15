#ifndef __RESPONSE_HPP__
# define __RESPONSE_HPP__

# include <map>
# include <string>
# include <vector>
# include <iostream>
#include <stdio.h>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

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


	struct ResponseLine 
	{
		std::string httpVersion;
		std::string statusCode;
		std::string reasonPhrase;
	};

	int const &getFdCGI() const;
	std::string &getResponse();
	bool isComplete() const;

	std::string createResponseLine(Request const &request, std::string const & reason = "");
	
	std::string getDefaultHeaders(Request const &request);

  protected:

  private:
	/* Typedefs*/
	typedef std::map<std::string, std::string> t_mapStrings;
	/* Members */
	std::string _buffer;
	int _cgiFd;
	bool _responseComplete;
	ResponseLine _responseLine;
	
	/* Methods */
	bool isError(Request const &request);
	int isCGI(Request const &request);
	std::string getFileContent(std::string const &pathname) const;
	char **headersToEnv(t_mapStrings const &headers) const;
};

#endif
