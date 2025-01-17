#include <string.h>
#include <sys/stat.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/socket.h>

#include "Response.hpp"

#define PARENT_END 0
#define CHILD_END 0

using std::ifstream;
using std::map;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecString;

namespace
{
	std::size_t const MAX_BUFFER_LEN = 8192;
}

namespace Utils
{
	std::size_t lenLongestPrefix(char const str1[], char const str2[]);
	bool isDirectory(char const pathname[]);
	string trimString(string const &input, string const &charset);
	string getListenAddress(t_vecString const &listenValue);
	string getListenPort(t_vecString const &listenValue);
}

// --- Public Methods --- //
Response::Response():
	_cgiFd(-1),
	_cgiPid(-1),
	_cgiFinished(false),
	_responseComplete(false)
{
}

Response::Response(Response const &src):
	_buffer(src._buffer),
	_cgiData(src._cgiData),
	_cgiFd(src._cgiFd),
	_cgiPid(src._cgiPid),
	_cgiFinished(src._cgiFinished),
	_responseComplete(src._responseComplete),
	_serverBlock(src._serverBlock),
	_locationBlock(src._locationBlock),
	_requestFile(src._requestFile),
	_requestQuery(src._requestQuery)
{
}

Response::Response(Request const &request, WebServer::Server const &configs):
	_cgiFd(-1),
	_cgiFinished(false),
	_responseComplete(false),
	_serverBlock(configs)
{
	if(isError(request))
	{
		std::cout<<"THERE IS ERROR"<<std::endl;
	}
	string requestTarget = request.getStartLine().requestTarget;
	string locationKey = getLocationBlock(requestTarget);
	if (locationKey.empty())
	{
		// TODO: set to error 404 not found in the response
		return;
	}
	splitRequestTarget(requestTarget);
	_locationBlock = configs._locations.at(locationKey);
	string pathname;
	t_vecString root = getValueOfLocation("root");
	if (root.empty())
		root = getValueOfServer("root");
	if (!root.empty())
		pathname += root.at(0) + '/';
	else
		_requestFile.erase(_requestFile.begin());

	pathname += _requestFile;
	if (Utils::isDirectory(pathname.c_str()))
	{
		t_vecString index = getValueOfLocation("index");
		if (index.empty())
			index = getValueOfServer("index");
		if (index.empty())
		{
			t_vecString autoIndex = getValueOfLocation("autoindex");
			if (autoIndex.empty())
				autoIndex = getValueOfServer("autoindex");
			if (autoIndex.empty())
			{
				// TODO: set an error in the response 
				return;
			}
			// TODO: create a response that list all files in the directory
			return;
		}
		pathname += index.at(0);
	}

	if(isCGI(configs._locations.at(locationKey)))
	{
		if (!handleCGI(request, getValueOfLocation("cgi")[0], pathname))
		{
			// TODO: set internal error in the response
		}
	}
	else
	{
		//if it is not cgi static response should be sent to client
	}
	string responseLine = createResponseLine(request);
	string responseHeaders = responseLine.append(getDefaultHeaders(request));
	string responseBody = getFileContent("web/www/index.html");
	string _buffer = responseHeaders.append(responseBody);
	std::cout << "fd check" << request.getFd()<<std::endl;
	std::cout<<"buffer :" << _buffer <<std::endl;
	int fd = request.getFd();
	send(fd, _buffer.c_str(), _buffer.size(), MSG_OOB);
}	

Response::~Response()
{
}

Response &Response::operator=(Response const &rhs)
{
	_buffer = rhs._buffer;
	_cgiData = rhs._cgiData;
	_cgiFd = rhs._cgiFd;
	_responseComplete = rhs._responseComplete;
	_cgiPid = rhs._cgiPid;
	_cgiFinished = rhs._cgiFinished;
	_responseComplete = rhs._responseComplete;
	_serverBlock = rhs._serverBlock;
	_locationBlock = rhs._locationBlock;
	_requestFile = rhs._requestFile;
	_requestQuery = rhs._requestQuery;
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

void Response::setEndCGI()
{
	_cgiFinished = true;
}

Response::e_IOReturn Response::retrieve()
{
	char buffer[MAX_BUFFER_LEN + 1];
	ssize_t bytes = recv(_cgiFd, buffer, MAX_BUFFER_LEN, 0);
	if (bytes == -1)
	{
		parseCGIResponse();
		return IO_ERROR;
	}

	if (bytes == 0)
	{
		parseCGIResponse();
		return IO_DISCONNECT;
	}

	_cgiData += buffer;
	return IO_SUCCESS;
}

std::string Response::createResponseLine(Request const &request, std::string const & reason)
{
	Request::e_phase phase = request.getPhase();
	if(phase == Request::PHASE_COMPLETE)
		_responseLine.statusCode = "200";
	else if(phase == Request::PHASE_ERROR)
		_responseLine.statusCode = "404";
	_responseLine.reasonPhrase = reason;
	_responseLine.httpVersion = "HTTP/1.1";

	std::string responseLine = _responseLine.httpVersion + " " + _responseLine.statusCode + " " + _responseLine.reasonPhrase + "\r\n";
	return responseLine;
}

std::string Response::getDefaultHeaders(Request const &request)
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
	std::string url = "Server: " + server.append(version) + "\r\n" + "Date: " + formattedGMT + "\r\n" + "Age: 0" + "\r\n";
	return url;
}

// --- Private Methods --- //root 
bool Response::handleCGI(Request const &request, string const &cgiExecutable, string const &pathname)
{
	int	socketPairFds[2];
	if (socketpair(AF_INET, SOCK_STREAM, 0, socketPairFds) == -1)
	{
		std::perror("socketpair");
		return false;
	}
	_cgiPid = fork();
	if (_cgiPid == -1)
	{
		std::perror("fork");
		close(socketPairFds[PARENT_END]);
		close(socketPairFds[CHILD_END]);
		return false;
	}
	if (_cgiPid == 0)
	{
		char *argv[3] = {NULL, NULL, NULL};
		close(socketPairFds[PARENT_END]);
		MyFileDescriptor autoCloseFd(socketPairFds[CHILD_END]);

		t_mapStrings headers = request.getHeaders();
		t_mapStrings cgiHeaders;
		cgiHeaders["REQUEST_METHOD"] = request.getStartLine().method;
		cgiHeaders["QUERY_STRING"] = _requestQuery;
		cgiHeaders["REDIRECT_STATUS"] = "200";
		cgiHeaders["SCRIPT_NAME"] = _requestFile;
		cgiHeaders["PATH_TRANSLATED"] = pathname;
		cgiHeaders["PATH_INFO"] = _requestFile;
		cgiHeaders["GATEWAY_INTERFACE"] = "CGI/1.1";
		cgiHeaders["SERVER_SOFTWARE"] = "ft_webserv/1.0";
		string listenAddress = "127.0.0.1";
		string listenPort = "8080";
		t_vecString listenValue = getValueOfServer("listen");
		if (listenValue.size() != 0)
		{
			listenAddress = Utils::getListenAddress(listenValue);
			listenPort = Utils::getListenPort(listenValue);
		}
		cgiHeaders["SERVER_NAME"] = listenAddress;
		cgiHeaders["SERVER_PORT"] = listenPort;
		cgiHeaders["SERVER_PROTOCOL"] = "HTTP/1.1";

		char **envp = headersToEnv(headers, cgiHeaders);
		if (envp == NULL)
			goto endCGI;

		argv[0] = strdup(cgiExecutable.c_str());
		argv[1] = strdup(pathname.c_str());
		if (argv[0] == NULL || argv[1] == NULL)
			goto endCGI;
		execve(argv[0], argv, envp);
endCGI:
		free(argv[0]);
		free(argv[1]);
		throw std::runtime_error(std::strerror(errno));
	}
	close(socketPairFds[CHILD_END]);
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

int Response::isCGI(WebServer::Location const &location) const
{
	typedef map<string, t_vecString> t_mapStringVecString;
	t_mapStringVecString::const_iterator cgi = location._pairs.find("cgi");
	return cgi != location._pairs.end() && cgi->second.size() != 0;
}

// /board/www/abc/index.html 
string Response::getFileContent(string const &pathname) const
{
	ifstream input(pathname.c_str());
	if (!input.is_open())
		return string();
	stringstream content;
	content << input.rdbuf();
	return content.str();
}

char **Response::headersToEnv(t_mapStrings const &headers, t_mapStrings const &cgiHeaders) const
{
	std::size_t len = headers.size() + cgiHeaders.size();
	char **envp = static_cast<char **>(std::malloc((len + 1) * sizeof(char *)));
	if (envp == NULL)
		return envp;

	std::size_t i = 0;
	for (t_mapStrings::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		string environmentVariable;
		if (it->first != "CONTENT_TYPE" && it->first != "CONTENT_LENGTH")
			environmentVariable = "HTTP_";

		environmentVariable += it->first + '=' + it->second;

		envp[i] = strdup(environmentVariable.c_str());
		if (envp[i] == NULL)
		{
			for (std::size_t j = 0; j < i; ++j)
				free(envp[j]);
			std::free(envp);
			return NULL;
		}
		std::memcpy(envp[i], environmentVariable.c_str(), environmentVariable.size() + 1);
		++i;
	}
	for (t_mapStrings::const_iterator it = cgiHeaders.begin(); it != cgiHeaders.end(); ++it)
	{
		string environmentVariable = it->first + '=' + it->second;
		envp[i] = strdup(environmentVariable.c_str());
		if (envp[i] == NULL)
		{
			for (std::size_t j = 0; j < i; ++j)
				free(envp[j]);
			std::free(envp);
			return NULL;
		}
		++i;
	}
	envp[i] = NULL;
	return envp;
}

string Response::getLocationBlock(std::string const &requestTarget) const
{
	typedef map<string, WebServer::Location> t_mapStringLoc;

	t_mapStringLoc::const_iterator locationBlock;
	std::size_t lenMax = 0;

	for (t_mapStringLoc::const_iterator it = _serverBlock._locations.begin(); it != _serverBlock._locations.end(); ++it)
	{
		string const &locationValue = it->first;
		std::size_t lenPrefix = 0;
		if (locationValue.at(locationValue.size() - 1) == '/')
			lenPrefix = Utils::lenLongestPrefix(locationValue.c_str(), requestTarget.c_str());
		else if (std::strcmp(locationValue.c_str(), requestTarget.c_str()) == 0)
			return locationValue;
		if (lenPrefix > lenMax)
		{
			locationBlock = it;
			lenMax = lenPrefix;
		}
	}

	if (lenMax != 0)
		return locationBlock->first;
	return string("");
}

Response::t_vecString Response::getValueOfLocation(string const &target)
{
	typedef map<string, t_vecString> t_mapStringVecString;

	t_mapStringVecString::const_iterator keyValues = _locationBlock._pairs.find(target);
	if (keyValues == _locationBlock._pairs.end())
		return t_vecString();
	return keyValues->second;
}

Response::t_vecString Response::getValueOfServer(string const &target)
{
	typedef map<string, t_vecString> t_mapStringVecString;

	t_mapStringVecString::const_iterator keyValues = _serverBlock._configs.find(target);
	if (keyValues == _serverBlock._configs.end())
		return t_vecString();
	return keyValues->second;
}

void Response::splitRequestTarget(string const &requestTarget)
{
	std::size_t pos = requestTarget.find("?");
	_requestFile = requestTarget.substr(0, pos);
	if (pos == string::npos)
		return;
	_requestQuery = requestTarget.substr(pos + 1);
}

void Response::parseCGIResponse()
{
	t_mapStrings cgiHeaders;
	std::size_t pos = _cgiData.find("\n");
	while (pos != 0)
	{
		string headerLine = _cgiData.substr(0, pos);
		_cgiData.erase(_cgiData.begin(), _cgiData.begin() + pos + 1);
		pos = headerLine.find(":");
		if (pos != string::npos)
		{
			string fieldName = headerLine.substr(0, pos);
			string fieldValue = Utils::trimString(headerLine.substr(pos + 1), " \t");
			cgiHeaders[fieldName] = fieldValue;
		}
		pos = _cgiData.find("\n");
	}
	_cgiData.erase(_cgiData.begin(), _cgiData.begin() + pos + 1);
	if (cgiHeaders.find("Status") != cgiHeaders.end())
	{
		_buffer = "HTTP/1.1 " + cgiHeaders["Status"] + "\r\n";
		cgiHeaders.erase("Status");
	}
	else
		_buffer = "HTTP/1.1 200 OK\r\n";
	// _buffer += getDefaultHeaders() + "\r\n";
	for (t_mapStrings::const_iterator it = cgiHeaders.begin(); it != cgiHeaders.end(); ++it)
		_buffer += it->first + ": " + it->second + "\r\n";
	{
		stringstream ss;
		ss << _cgiData.size();
		_buffer += "Content-Length: " + ss.str() + "\r\n";
	}
	_buffer += "\r\n" + _cgiData;
	string().swap(_cgiData);
	_responseComplete = true;
}

MyFileDescriptor::MyFileDescriptor(int fd):
	_fd(fd)
{ }
MyFileDescriptor::~MyFileDescriptor()
{ close(_fd); }
MyFileDescriptor::MyFileDescriptor():
	_fd(-1)
{ }
MyFileDescriptor::MyFileDescriptor(MyFileDescriptor const &):
	_fd(-1)
{ }
MyFileDescriptor &MyFileDescriptor::operator=(MyFileDescriptor const &)
{ _fd = -1; return *this; }

std::size_t Utils::lenLongestPrefix(char const str1[], char const str2[])
{
	std::size_t i = 0;
	while (str1[i] != '\0' && str2[i] != '\0')
	{
		if (str1[i] != str2[i])
			break;
		++i;
	}
	return i;
}

bool Utils::isDirectory(char const pathname[])
{
	struct stat s;
	if (stat(pathname, &s) == -1)
		return false;

	return !!(s.st_mode & S_IFDIR);
}