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
		O_NOT_READY,
		O_INCOMPLETE,
		IO_SUCCESS,
	};

	/* Methods */
	Response();
	Response(Response const &src);
	Response(Request const &request, Server const &config);
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
	e_IOReturn sendData(int fd);

  protected:

  private:
	/* Typedefs*/
	typedef std::map<std::string, std::string> t_mapStrings;
	typedef std::vector<std::string> t_vecStrings;
	/* Members */
	std::string _buffer;
	std::string _cgiData;
	int _cgiFd;
	pid_t _cgiPid;
	ResponseLine _responseLine;
	std::string _headers;
	Server _serverBlock;
	std::string _locationKey;
	Location _locationBlock;
	std::string _requestFile;
	std::string _requestQuery;
	std::string _absolutePath;
	t_mapStrings _contentType;
	
	/* Methods */
	std::string createResponseLine(Request const &request, std::string const & reason = "");
	std::string getDefaultHeaders(std::size_t size);
	bool isError(Request const &request);
	int isCGI() const;
	std::string getFileContent(std::string const &pathname) const;
	char **headersToEnv(t_mapStrings const &headers, t_mapStrings const &cgiHeaders) const;
	void splitRequestTarget(std::string const &requestTarget);
	bool handleCGI(Request const &request);
	void parseCGIResponse();
	int setLocationBlock(Request const &request);
	int setAbsolutePathname();
	t_mapStrings createCGIHeaders(Request const &request);
	t_vecStrings getValuesOf(std::string const &target);
	std::string getValueOf(std::string const &target);
	void initContentType();
	std::string getContentType(std::string const &file);
	void handleRedirection();
	std::string createStartLine(int statusCode, std::string const &reason = "");
	void handleUpload(Request const &request);
	void handleDelete(Request const &request);
	void createBuffer(int statusCode, std::string const &reason = "");
	std::string getClosedHeaders(std::size_t size);
	void createErrorResponse(Request const &request);
	std::string listDirectory(Request const &request);
};

#endif
