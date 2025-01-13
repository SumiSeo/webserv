#ifndef __RESPONSE_HPP__
# define __RESPONSE_HPP__

# include <map>
# include <string>
# include <vector>
# include <iostream>

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

	//this function is checking if it returns -1 or fd
	int const &getFdCGI() const;

	// It returns _buffer(response)
	std::string &getResponse();

	// if all response functionality is done or not , it is returing true or false
	bool isComplete() const;

	// it is for creating status-line of response
	std::string createResponseLine(e_statusCode code, std::string const & reason = "");

	// "hard code" headers 
	// Server: ft_webvserv/1.0\r\ndate: [insert the date with this format]\r\nage: 0\r\n".
	std::string getDefaultHeaders();

  protected:

  private:
	/* Typedefs*/
	typedef std::map<std::string, std::string> t_mapStrings;
	/* Members */
	std::string _buffer;
	int _cgiFd;
	bool _responseComplete;

	/* Methods */

	
	//check phase and status
	bool isError(Request const &request);

	//extension check to know if it is static or cgi .html or .py/ .php
	int isCGI();





};

#endif
