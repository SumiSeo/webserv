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

	// Main Methods

	//this function is checking if it returns -1 or fd
	int const &getFdCGI() const;

	// It returns _buffer(response)
	std::string &getResponse();

	// if all response functionality is done or not , it is returing true or false
	bool isComplete() const;

	ResponseLine createResponseLine(Request const &request, std::string const & reason = "");
	
	// Server: ft_webvserv/1.0\r\ndate: [insert the date with this format]\r\nage: 0\r\n".
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
	std::string _headers;
	
	/* Methods */
	bool isError(Request const &request);
	int isCGI(Request const &request);





};

#endif
