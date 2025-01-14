#include "Response.hpp"

#define PARENT_END 0
#define CHILD_END 0

using std::string;
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
	(void)request;
	(void)configs;
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
