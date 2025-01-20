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
	/* New Variable Types */
	enum e_IOReturn
	{
		IO_ERROR,
		IO_DISCONNECT,
		IO_SUCCESS,
	};

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
	void setEndCGI();
	e_IOReturn retrieve();

	std::string createResponseLine(Request const &request, std::string const & reason = "");
	
	std::string getDefaultHeaders(Request const &request,int size);

  protected:

  private:
	/* Typedefs*/
	typedef std::map<std::string, std::string> t_mapStrings;
	typedef std::vector<std::string> t_vecString;
	/* Members */
	std::string _buffer;
	std::string _cgiData;
	int _cgiFd;
	pid_t _cgiPid;
	bool _cgiFinished;
	bool _responseComplete;
	ResponseLine _responseLine;
	std::string _headers;
	WebServer::Server _serverBlock;
	std::string _locationKey;
	WebServer::Location _locationBlock;
	std::string _requestFile;
	std::string _requestQuery;
	std::string _absolutePath;
	t_mapStrings _contentType;
	
	/* Methods */
	bool handleCGI(Request const &request, std::string const &cgiExecutable);
	bool isError(Request const &request);
	int isCGI() const;
	std::string getFileContent(std::string const &pathname) const;
	char **headersToEnv(t_mapStrings const &headers, t_mapStrings const &cgiHeaders) const;
	std::string getLocationBlock(std::string const &requestTarget) const;
	t_vecString getValueOfLocation(std::string const &target);
	t_vecString getValueOfServer(std::string const &target);
	void splitRequestTarget(std::string const &requestTarget);
	void parseCGIResponse();
	int setLocationBlock(Request const &request);
	int setAbsolutePathname();
	t_mapStrings createCGIHeaders(Request const &request);
	t_vecString getValueOf(std::string const &target);
	void initContentType();
	std::string getContentType(std::string const &file);
};

#endif
