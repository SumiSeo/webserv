#include "Response.hpp"

using std::string;
using std::vector;

// --- Public Methods --- //
Response::Response():
	_cgiFd(-1),
	_responseComplete(false)
{
}

Response::Response(Request const &request, WebServer::Server const &configs):
	_cgiFd(-1),
	_responseComplete(false)
{


	std::cout<<"Response constructor called" << std::endl;
	(void)configs;
	if(!isError(request))
	{
		std::cout<<"THERE IS NO ERROR"<<std::endl;
	}
	// if request is valid and doesn't require a cgi, create a response that answer the request
	// if request is invalid, create a response with the error code
	// if request needs a cgi, create a fork to launch the cgi
}

Response::~Response()
{
}

Response &Response::operator=(Response const &rhs)
{
	_buffer = rhs._buffer;
	_cgiFd = rhs._cgiFd;
	_responseComplete = rhs._responseComplete;
	return *this;
}

int const &Response::getFdCGI() const
{
	return _cgiFd;
}

string &Response::getResponse()
{
	return _buffer;
}

bool Response::isComplete() const
{
	return _responseComplete;
}

std::string Response::createResponseLine(e_statusCode code, std::string const & reason)
{
	std::cout<<"Create responseline called"<<std::endl;
(void)code;
(void)reason;
return "hi";
}

std::string Response::getDefaultHeaders()
{
return "bye";
}

// --- Private Methods --- //
Response::Response(Response const &src):
	_buffer(src._buffer),
	_cgiFd(src._cgiFd),
	_responseComplete(src._responseComplete)
{
}

bool Response::isError(Request const &request)
{
	request.getPhase();
	request.getStatusCode();
    return false;
}