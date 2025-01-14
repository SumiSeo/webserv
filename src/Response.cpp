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
	if(isError(request))
	{
		std::cout<<"THERE IS ERROR"<<std::endl;
	}
	if(isCGI(request))
	{
		std::cout<<"IS IT CGI ? "<<std::endl;
	}
	createResponseLine(request);
	getDefaultHeaders(request);
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

Response::ResponseLine Response::createResponseLine(Request const &request, std::string const & reason)
{
	_responseLine.statusCode = request.getStatusCode();
	_responseLine.reasonPhrase = reason;
	_responseLine.httpVersion = request.getStartLine().httpVersion;

	std::cout << "response code :" << _responseLine.statusCode <<std::endl;
	std::cout << "response reason :" << _responseLine.reasonPhrase <<std::endl;
	std::cout << "response httpVersion :" << _responseLine.httpVersion <<std::endl;
	return _responseLine;
}

std::string Response::getDefaultHeaders(Request const &request)
{
	//ft_webvserv/1.0\r\ndate: [insert the date with this format]\r\nage: 0\r\n".

	// HTTP/1.1 200
	// Content-Type: text/html
	// Date: Tue, 29 Oct 2024 16:56:32 GMT
	time_t now;
	time(&now);
	std::cout<<"time stamp "<< now <<std::endl; 

	tm *ltm = localtime(&now);

    char formatted[100];
	std::strftime(formatted, sizeof(formatted), "%a, %d %b %Y %H:%M:%S ", ltm);
	std::string formattedDate = formatted;
	std::string formattedGMT = formattedDate.append("GMT");
	std::cout  << formatted<<std::endl;
	std::cout  << formattedGMT<<std::endl;
	std::string server = "ft_webserv";
	std::string version = "/" + request.getStartLine().httpVersion;
	std::string url = server.append(version) + "\r\n";
	std::cout<<"url check : " << url<<std::endl; 
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
	if(request.getPhase() == Request::PHASE_ERROR)
	{
		std::cout<<"Phase error"<< request.getPhase()<<std::endl;
		return true;
	}
	if(request.getStatusCode()!= VALID && request.getStatusCode() >1 )
	{
	   std::cout<<"status code error"<<request.getStatusCode()<<std::endl;
	   return true;
	}
	return false;
}

int Response::isCGI(Request const &request)
{
	std::string targetExten = request.getStartLine().requestTarget;
	std::string py = ".py";
	std::string php = ".php";
	std::cout<<targetExten<<std::endl;
	if(strstr(targetExten.c_str(),py.c_str()) || strstr(targetExten.c_str(), php.c_str()))
	{
		//should response with cgi call
		std::cout<<"is cgi called : "<< targetExten <<std::endl;
	}
	else
	{
		// should response with normal static call 
	}
	
	return 1;
}