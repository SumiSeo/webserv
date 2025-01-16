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
		int statusCode;
		std::string reasonPhrase;
	};

	int const &getFdCGI() const;
	std::string &getResponse();
	bool isComplete() const;
	void setEndCGI();
	e_IOReturn retrieve();

	void createResponseLine(Request const &request, std::string const & reason = "");
	
	void getDefaultHeaders(Request const &request);

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
	WebServer::Location _locationBlock;
	std::string _requestFile;
	std::string _requestQuery;
	
	/* Methods */
	bool handleCGI(Request const &request, std::string const &cgiExecutable, std::string const &pathname);
	bool isError(Request const &request);
	int isCGI(WebServer::Location const &location) const;
	std::string getFileContent(std::string const &pathname) const;
	char **headersToEnv(t_mapStrings const &headers, t_mapStrings const &cgiHeaders) const;
	std::string getLocationBlock(std::string const &requestTarget) const;
	t_vecString getValueOfLocation(std::string const &target);
	t_vecString getValueOfServer(std::string const &target);
	void splitRequestTarget(std::string const &requestTarget);
	void parseCGIResponse();
};

class MyFileDescriptor
{
public:
	MyFileDescriptor(int fd);
	~MyFileDescriptor();

private:
	int _fd;

	MyFileDescriptor();
	MyFileDescriptor(MyFileDescriptor const &src);

	MyFileDescriptor &operator=(MyFileDescriptor const &rhs);
};

#endif
