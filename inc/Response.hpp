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
		int statusCode;
		std::string reasonPhrase;
	};

	int const &getFdCGI() const;
	std::string &getResponse();
	bool isComplete() const;

	void createResponseLine(Request const &request, std::string const & reason = "");
	
	void getDefaultHeaders(Request const &request);

  protected:

  private:
	/* Typedefs*/
	typedef std::map<std::string, std::string> t_mapStrings;
	/* Members */
	std::string _buffer;
	int _cgiFd;
	bool _responseComplete;
	ResponseLine _responseLine;
	std::string _headers;
	
	/* Methods */
	bool handleCGI(Request const &request, WebServer::Server const &config, std::string const &pathname);
	bool isError(Request const &request);
	int isCGI(Request const &request);
	std::string getFileContent(std::string const &pathname) const;
	char **headersToEnv(t_mapStrings const &headers) const;
};

#endif
