#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#include "Response.hpp"

#define PARENT_END 0
#define CHILD_END 0

using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

// --- Public Methods --- //
Response::Response():
	_cgiFd(-1),
	_responseComplete(false)
{
}

Response::Response(Response const &src):
	_buffer(src._buffer),
	_cgiFd(src._cgiFd),
	_responseComplete(src._responseComplete)
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
bool Response::handleCGI(Request const &request, WebServer::Server const &config, string const &pathname)
{
	if (config._configs.find("cgi") == config._configs.end())
		return false;

	t_vecString const &cgiExecutable = config._configs["cgi"];
	if (cgiExecutable.size() == 0)
		return false;

	int	socketPairFds[2];
	if (socketpair(AF_INET, SOCK_STREAM, 0, socketPairFds) == -1)
	{
		std::perror("socketpair");
		return false;
	}
	pid_t fpid = fork();
	if (fpid == -1)
	{
		std::perror("fork");
		close(socketPairFds[PARENT_END]);
		close(socketPairFds[CHILD_END]);
		return false;
	}
	if (fpid == 0)
	{
		close(socketPairFds[PARENT_END]);
		t_mapStrings headers = request.getHeaders();
		char const argv[3] = {
			cgiExecutable[0].c_str(),
			pathname.c_str(),
			NULL;
		}
		execve(argv[0], argv[1], NULL);
		throw std::runtime_error(std::strerror(errno));
	}
	close(socketpairFds[CHILD_END]);
	_cgiFd = socketPairFds[PARENT_END];
	return true;
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
string Response::getFileContent(string const &pathname) const
{
	ifstream input(pathname.c_str());
	if (!input.is_open())
		return string();
	stringstream content;
	content << input.rdbuf();
	return content.str();
}

char **Response::headersToEnv(t_mapStrings const &headers) const
{
	std::size_t len = headers.size();
	char **envp = new(std::nothrow) char*[len + 1];
	if (envp == NULL)
		return envp;

	std::size_t i = 0;
	for (t_mapStrings::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		string environmentVariable = "HTTP_" + it->first + '=' + it->second;
		envp[i] = new(std::nothrow) char[environmentVariable.size() + 1];
		if (envp[i] == NULL)
		{
			for (std::size_t j = 0; j < i; ++j)
				delete envp[j];
			delete[] envp;
			return NULL;
		}
		std::memcpy(envp[i], environmentVariable.c_str(), environmentVariable.size() + 1);
		++i;
	}
	envp[i] = NULL;
	return envp;
}
