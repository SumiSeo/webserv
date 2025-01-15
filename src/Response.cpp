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
	(void)configs;
	if(isError(request))
	{
		std::cout<<"THERE IS ERROR"<<std::endl;
	}
	if(isCGI(request))
	{
		std::cout<<"IS IT CGI ? "<<std::endl;
	}
	else
	{
		//if it is not cgi static response should be sent to client
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

void Response::createResponseLine(Request const &request, std::string const & reason)
{
	_responseLine.statusCode = request.getStatusCode();
	_responseLine.reasonPhrase = reason;
	_responseLine.httpVersion = request.getStartLine().httpVersion;
}

void Response::getDefaultHeaders(Request const &request)
{
	time_t now;
	time(&now);
	tm *ltm = localtime(&now);
    char formatted[100];
	std::strftime(formatted, sizeof(formatted), "%a, %d %b %Y %H:%M:%S ", ltm);
	std::string formattedDate = formatted;
	std::string formattedGMT = formattedDate.append("GMT");
	std::string server = "ft_webserv";
	std::string version = "/" + request.getStartLine().httpVersion;
	std::string url = server.append(version) + "\r\n" + formattedGMT + "\r\n" + "age: 0" + "\r\n";
	_headers = url;
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
		return 1;

	}
	return 0;
	
}