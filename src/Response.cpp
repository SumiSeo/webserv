#include <string.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/socket.h>

#include "Response.hpp"

#define PARENT_END 0
#define CHILD_END 1

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
	bool isDirectory(char const pathname[]);
	string trimString(string const &input, string const &charset);
	string getListenAddress(t_vecString const &listenValue);
	string getListenPort(t_vecString const &listenValue);
}

// --- Public Methods --- //
Response::Response():
	_cgiFd(-1),
	_cgiPid(-1)
{
}

Response::Response(Response const &src):
	_buffer(src._buffer),
	_cgiData(src._cgiData),
	_cgiFd(src._cgiFd),
	_cgiPid(src._cgiPid),
	_serverBlock(src._serverBlock),
	_locationKey(src._locationKey),
	_locationBlock(src._locationBlock),
	_requestFile(src._requestFile),
	_requestQuery(src._requestQuery),
	_absolutePath(src._absolutePath)
{
}

Response::Response(Request const &request, Server const &configs):
	_cgiFd(-1),
	_serverBlock(configs)
{
	initContentType();
	if(isError(request))
	{
		std::cout<<"THERE IS ERROR"<<std::endl;
	}
	splitRequestTarget(request.getStartLine().requestTarget);
	_locationKey = request.getLocationKey();
	_locationBlock = _serverBlock._locations.at(_locationKey);
	if (setAbsolutePathname() != 0)
	{
		// TODO: set to error 404 not found in the response
		return;
	}
	if (Utils::isDirectory(_absolutePath.c_str()))
	{
		string index = getValueOf("index");
		if (index.empty())
		{
			string autoIndex = getValueOf("autoindex");
			if (autoIndex.empty() || autoIndex == "off")
			{
				// TODO: set an error in the response 
				return;
			}
			// TODO: create a response that list all files in the directory
			return;
		}
		_absolutePath += index;
	}

	if(isCGI())
	{
		if (!handleCGI(request, _locationBlock.getValueOf("cgi")))
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
	std::cout << "ABSOLUTE PATH"  << _absolutePath<< std::endl;
	string responseBody = getFileContent(_absolutePath);
	string _buffer = responseHeaders.append(responseBody);
	std::cout << "fd check" << request.getFd()<<std::endl;
	std::cout<<"buffer :" << _buffer <<std::endl;
	int fd = request.getFd();
	send(fd, _buffer.c_str(), _buffer.size(), 0);
}

Response::~Response()
{
}

Response &Response::operator=(Response const &rhs)
{
	_buffer = rhs._buffer;
	_cgiData = rhs._cgiData;
	_cgiFd = rhs._cgiFd;
	_cgiPid = rhs._cgiPid;
	_serverBlock = rhs._serverBlock;
	_locationKey = rhs._locationKey;
	_locationBlock = rhs._locationBlock;
	_requestFile = rhs._requestFile;
	_requestQuery = rhs._requestQuery;
	_absolutePath = rhs._absolutePath;
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

void Response::setEndCGI()
{
	_cgiFd = -1;
}

Response::e_IOReturn Response::retrieve()
{
	char buffer[MAX_BUFFER_LEN + 1];
	ssize_t bytes = recv(_cgiFd, buffer, MAX_BUFFER_LEN, 0);
	if (bytes == -1)
	{
		_cgiData = "Status: 500 Internal Server Error\n";
		return IO_ERROR;
	}

	if (bytes == 0)
	{
		if (_cgiData.empty())
			_cgiData = "Status: 500 Internal Server Error\n";
		return IO_DISCONNECT;
	}

	buffer[bytes] = '\0';

	_cgiData += buffer;
	return IO_SUCCESS;
}

Response::e_IOReturn Response::sendData(int fd)
{
	if (_cgiFd != -1)
		return O_NOT_READY;
	if (!_cgiData.empty())
		parseCGIResponse();
	ssize_t bytes = send(fd, _buffer.c_str(), _buffer.size(), MSG_NOSIGNAL);

	if (bytes == -1)
		return IO_ERROR;

	if (_buffer.size() != 0 && bytes == 0)
		return IO_DISCONNECT;

	if (static_cast<std::size_t>(bytes) != _buffer.size())
		return O_INCOMPLETE;

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
	std::string contentType = getContentType("index.html");
	std::string server = "ft_webserv";
	std::string version = "/" + request.getStartLine().httpVersion;
	std::string url = "Server: " + server.append(version) + "\r\n" + "Content-type: " + contentType + "\r\n" "Date: " + formattedGMT + "\r\n" + "Age: 0" + "\r\n"  + "\r\n";
	return url;
}

// --- Private Methods --- //root 
bool Response::handleCGI(Request const &request, string const &cgiExecutable)
{
	int	socketPairFds[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPairFds) == -1)
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
		close(socketPairFds[PARENT_END]);
		char *argv[3] = {NULL, NULL, NULL};
		if (dup2(socketPairFds[CHILD_END], STDOUT_FILENO) == -1)
		{
			std::perror("dup2");
			close(socketPairFds[CHILD_END]);
			throw std::runtime_error("dup2 failed");
		}
		close(socketPairFds[CHILD_END]);

		if (request.getBody().size() != 0)
		{
			char nameTemplate[] = "/tmp/body_request_cgi_XXXXXX";
			int bodyFd = mkstemp(nameTemplate);

			if (bodyFd == -1)
			{
				std::perror("mkstemp");
				throw std::runtime_error("mkstemp failed");
			}
			if (write(bodyFd, request.getBody().c_str(), request.getBody().size()) == -1)
			{
				std::perror("write");
				close(bodyFd);
				throw std::runtime_error("write failed");
			}
			if (lseek(bodyFd, 0, SEEK_SET) == -1)
			{
				std::perror("lseek");
				close(bodyFd);
				throw std::runtime_error("lseek failed");
			}
			if (dup2(bodyFd, STDIN_FILENO) == -1)
			{
				std::perror("dup2");
				close(bodyFd);
				throw std::runtime_error("dup2 failed");
			}
			close(bodyFd);
		}

		t_mapStrings headers = request.getHeaders();
		t_mapStrings cgiHeaders = createCGIHeaders(request);

		char **envp = headersToEnv(headers, cgiHeaders);
		if (envp == NULL)
			goto endCGI;

		argv[0] = strdup(cgiExecutable.c_str());
		argv[1] = strdup(_absolutePath.c_str());
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

int Response::isCGI() const
{
	typedef map<string, t_vecString> t_mapStringVecString;
	t_mapStringVecString::const_iterator cgi = _locationBlock._pairs.find("cgi");
	return cgi != _locationBlock._pairs.end() && cgi->second.size() != 0;
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
	while (pos != string::npos && pos != 0)
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
	if (pos != string::npos)
		_cgiData.erase(_cgiData.begin(), _cgiData.begin() + pos + 1);
	else
		_cgiData.erase(_cgiData.begin(), _cgiData.end());
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
}

int Response::setAbsolutePathname()
{
	string root = getValueOf("root");
	if (root.empty())
		return 1;
	_absolutePath += root + _requestFile.substr(_locationKey.size());
	return 0;
}

Response::t_mapStrings Response::createCGIHeaders(Request const &request)
{
	t_mapStrings cgiHeaders;
	cgiHeaders["REQUEST_METHOD"] = request.getStartLine().method;
	cgiHeaders["QUERY_STRING"] = _requestQuery;
	cgiHeaders["REDIRECT_STATUS"] = "200";
	cgiHeaders["SCRIPT_NAME"] = _requestFile;
	cgiHeaders["PATH_TRANSLATED"] = _absolutePath;
	cgiHeaders["PATH_INFO"] = _requestFile;
	cgiHeaders["GATEWAY_INTERFACE"] = "CGI/1.1";
	cgiHeaders["SERVER_SOFTWARE"] = "ft_webserv/1.0";
	string listenAddress = "127.0.0.1";
	string listenPort = "8080";
	t_vecString listenValue = _serverBlock.getValuesOf("listen");
	if (listenValue.size() != 0)
	{
		listenAddress = Utils::getListenAddress(listenValue);
		listenPort = Utils::getListenPort(listenValue);
	}
	cgiHeaders["SERVER_NAME"] = listenAddress;
	cgiHeaders["SERVER_PORT"] = listenPort;
	cgiHeaders["SERVER_PROTOCOL"] = "HTTP/1.1";
	cgiHeaders["REMOTE_ADDR"] = request.getAddress();
	cgiHeaders["SERVER_PORT"] = listenPort;
	return cgiHeaders;
}

Response::t_vecString Response::getValuesOf(string const &target)
{
	t_vecString values = _locationBlock.getValuesOf(target);
	if (values.empty())
		values = _serverBlock.getValuesOf(target);
	return values;
}

string Response::getValueOf(string const &target)
{
	string value = _locationBlock.getValueOf(target);
	if (value.empty())
		value = _serverBlock.getValueOf(target);
	return value;
}

void Response::initContentType()
{
	_contentType["html"] = "text/html";
	_contentType["css"] = "text/css";
	_contentType["xml"] = "text/xml";
	_contentType["txt"] = "text/plain";
	_contentType["gif"] = "image/gif";
	_contentType["jpg"] = "image/jpeg";
	_contentType["jpeg"] = "image/jpeg";
	_contentType["png"] = "image/png";
	_contentType["ico"] = "image/x-ico";
	_contentType["bmp"] = "image/x-ms-bmp";
	_contentType["webp"] = "image/webp";
	_contentType["mp3"] = "audio/mpeg";
	_contentType["ogg"] = "audio/ogg";
	_contentType["m4a"] = "audio/x-m4a";
	_contentType["mp4"] = "video/mp4";
	_contentType["mpg"] = "video/mpeg";
	_contentType["mpeg"] = "video/mpeg";
	_contentType["mov"] = "video/quicktime";
	_contentType["webm"] = "video/webm";
	_contentType["avi"] = "video/x-msvideo";
	_contentType["js"] = "application/javascript";
	_contentType["json"] = "application/json";
	_contentType["pdf"] = "application/pdf";
	_contentType["zip"] = "application/zip";
}

string Response::getContentType(string const &file)
{
	std::size_t pos = file.rfind(".");
	if (pos == string::npos)
		return "application/octet-stream";
	string extension = file.substr(pos + 1);
	if (_contentType.find(extension) == _contentType.end())
		return "application/octet-stream";
	return _contentType[extension];
}

bool Utils::isDirectory(char const pathname[])
{
	struct stat s;
	if (stat(pathname, &s) == -1)
		return false;

	return !!(s.st_mode & S_IFDIR);
}
