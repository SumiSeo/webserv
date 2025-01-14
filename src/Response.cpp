#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#include "Response.hpp"

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
Response::Response(Response const &src):
	_buffer(src._buffer),
	_cgiFd(src._cgiFd),
	_responseComplete(src._responseComplete)
{
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
