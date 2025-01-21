#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <limits>
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
using std::ofstream;
using std::map;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecStrings;

namespace
{
	std::size_t const MAX_BUFFER_LEN = 8192;
	template<typename T> string numToString(T number);
	bool isValidPathToDelete(string const &path);
	t_vecStrings split(string const &str, char sep);
}

namespace Utils
{
	bool isDirectory(char const pathname[]);
	string trimString(string const &input, string const &charset);
	string getListenAddress(t_vecStrings const &listenValue);
	string getListenPort(t_vecStrings const &listenValue);
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
		std::cout<<"THERE IS ERROR WITH STATUS CODE"<<std::endl;
		return;
	}
	splitRequestTarget(request.getStartLine().requestTarget);
	_locationKey = request.getLocationKey();
	_locationBlock = _serverBlock._locations.at(_locationKey);

	if (!getValuesOf("return").empty())
	{
		handleRedirection();
		return;
	}
	setAbsolutePathname();
	std::cout <<" @@@PATH CHECK " << _absolutePath << std::endl;
	if (Utils::isDirectory(_absolutePath.c_str()))
	{
		string autoIndex = getValueOf("autoindex");
		if (autoIndex.empty() || autoIndex != "on")
		{
			string responseLine = createStartLine(403, "Forbidden");
			string responseBody;
			string errorPath = _serverBlock.getErrorPage(403);
			if (errorPath.empty())
				responseBody = "Forbidden";
			else
				responseBody = getFileContent(errorPath);
			_absolutePath = ".txt";
			string responseHeaders = getDefaultHeaders(responseBody.size());
			_buffer = responseLine + "\r\n" + responseHeaders + "\r\n" + responseBody;
		}
		else
		{
			string responseLine;
			string responseBody = listDirectory(request);
			if (responseBody.empty())
				responseLine = createStartLine(200, "Ok");
			else
				responseLine = createStartLine(403, "Forbidden");

			_absolutePath = ".html";
			string responseHeaders = getDefaultHeaders(responseBody.size());
			_buffer = responseLine + "\r\n" + responseHeaders + "\r\n" + responseBody;
		}
		return;
	}
	if(isCGI())
	{
		if (!handleCGI(request))
		{
			// Pascal control if CGI fails
			// TODO: set internal error in the response
		}
	}
	else
	{
		string const &method = request.getStartLine().method;
		if (method == "GET")
		{
			string responseLine = createResponseLine(request);
			string responseBody = getFileContent(_absolutePath);
			int responseBodySize = responseBody.size();
			string responseHeaders = responseLine.append(getDefaultHeaders(responseBodySize));
			std::cout << "ABSOLUTE PATH"  << _absolutePath<< std::endl;
			std::cout << "HEADERS " << responseHeaders <<std::endl; 
			string responseHeadersLine = responseHeaders + "\r\n";
			_buffer = responseHeadersLine.append(responseBody);
			std::cout << "fd check" << request.getFd()<<std::endl;
			std::cout<<"buffer :" << _buffer <<std::endl;
		}
		else if (method == "POST")
		{
			if (!_locationBlock.getValueOf("upload_path").empty())
				handleUpload(request);
			else
			{
				// TODO: create response 405 Not Allowed
				//it ts is POST check and then if it is upload 
				// I have to display bad request // 
			}
		}
		else if (method == "DELETE")
		{
			if (!_locationBlock.getValueOf("upload_path").empty())
				handleDelete(request);
			else
			{
				// TODO: create response 405 Not Allowed
			}
		}
	}
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

std::string Response::getDefaultHeaders(std::size_t size)
{
	time_t now;
	time(&now);
	tm *ltm = localtime(&now);
    char formatted[100];
	std::strftime(formatted, sizeof(formatted), "%a, %d %b %Y %H:%M:%S ", ltm);
	std::string formattedDate = formatted;
	std::string formattedGMT = formattedDate.append("GMT");
	std::string contentType = getContentType(_absolutePath);
	std::string server = "ft_webserv";
	std::string version = "/1.0";
	stringstream convertedSize;
	convertedSize << size;
	string finalSize = convertedSize.str();
	std::string url = "Server: " + server.append(version) + "\r\n" + "Content-type: " + contentType + "\r\n" +"Content-length: " + finalSize + "\r\n" "Date: " + formattedGMT + "\r\n" + "Age: 0" + "\r\n";
	return url;
}

// --- Private Methods --- //root 
bool Response::handleCGI(Request const &request)
{
	int	socketPairFds[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPairFds) == -1)
	{
		std::perror("socketpair");
		return false;
	}
	string cgiExecutable = _locationBlock.getValueOf("cgi");
	if (cgiExecutable.empty())
		cgiExecutable = "/usr/bin/php-cgi";
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
		string root = getValueOf("root");
		if (root.empty())
			root = "/";
		if (chdir(root.c_str()) == -1)
			std::perror("chdir");

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
	std::cout << "status code : " << request.getStatusCode()<<std::endl;
	if(request.getStatusCode() == OK || request.getStatusCode() == ACCEPTED || request.getStatusCode() == CREATED )
	   return false;
	return true;
}

int Response::isCGI() const
{
	string cgi = _locationBlock.getValueOf("cgi");
	if (!cgi.empty())
		return true;

	std::size_t pos = _absolutePath.rfind('.');
	if (pos == string::npos)
		return false;

	string extension = _absolutePath.substr(pos + 1);
	return extension == "php";
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
	_buffer += getDefaultHeaders(_cgiData.size()) + "\r\n";
	for (t_mapStrings::const_iterator it = cgiHeaders.begin(); it != cgiHeaders.end(); ++it)
		_buffer += it->first + ": " + it->second + "\r\n";
	_buffer += "\r\n" + _cgiData;
	string().swap(_cgiData);
}

int Response::setAbsolutePathname()
{
	string root = getValueOf("root");
	if (root.empty())
		return 1;
	_absolutePath += root + _requestFile.substr(_locationKey.size() - 1);
	if (_absolutePath[_absolutePath.size() - 1] == '/')
		_absolutePath += getValueOf("index");
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
	t_vecStrings listenValue = _serverBlock.getValuesOf("listen");
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

Response::t_vecStrings Response::getValuesOf(string const &target)
{
	t_vecStrings values = _locationBlock.getValuesOf(target);
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

void Response::handleRedirection()
{
	t_vecStrings redirection = getValuesOf("return");
	string startLine = createStartLine(std::atoi(redirection[0].c_str()));
	string bodyMsg = "Redirecting";
	_absolutePath = ".txt";
	string headers = getDefaultHeaders(bodyMsg.size());
	headers += "Location: " + redirection[1] + "\r\n";
	_buffer = startLine + "\r\n" + headers + "\r\n" + bodyMsg;
}

string Response::createStartLine(int statusCode, std::string const &reason)
{
	string startLine = "HTTP/1.1 " + numToString(statusCode) + ' ' + reason;
	return startLine;
}

void Response::handleUpload(Request const &request)
{
	string path = _locationBlock.getValueOf("upload_path");
	string fileName = "tmpfile";
	unsigned long num = 0;
	bool newFile = false;
	for (; num < std::numeric_limits<unsigned long>::max(); ++num)
	{
		string pathName = path + fileName + numToString(num);
		if (access(pathName.c_str(), F_OK) != 0)
		{
			newFile = true;
			break;
		}
	}
	if (newFile)
	{
		try
		{
			fileName += numToString(num);
			string pathName = path + fileName;
			ofstream fileOutput(pathName.c_str());
			fileOutput << request.getBody();
			string startLine = createStartLine(201, "Created");
			_absolutePath = ".txt";
			string headers = getDefaultHeaders(0);
			headers += "Location: " + _locationKey + fileName + "\r\n";
			_buffer = startLine + "\r\n" + headers + "\r\n";
			return;
		}
		catch (std::ios_base::failure const &f)
		{
			std::cerr << "handleUpload Error : " << f.what() << std::endl;
		}
	}
	string startLine = createStartLine(500, "Internal Server Error");
	string bodyMsg = "Internal Server Error";
	_absolutePath = ".txt";
	string headers = getDefaultHeaders(bodyMsg.size());

	_buffer = startLine + "\r\n" + headers + "\r\n" + bodyMsg;
}

void Response::handleDelete(Request const &request)
{
	string path = _locationBlock.getValueOf("upload_path");
	string fileName = request.getStartLine().requestTarget.substr(_locationKey.size() - 1);
	string pathName = path + fileName;
	if (isValidPathToDelete(pathName))
	{
		if (unlink(pathName.c_str()) == -1)
			std::perror("unlink");
	}
	string startLine = createStartLine(202, "Accepted");
	_absolutePath = ".txt";
	string headers = getDefaultHeaders(0);
	_buffer = startLine + "\r\n" + headers + "\r\n";
}

string Response::listDirectory(Request const &request)
{
	DIR *directory = opendir(_absolutePath.c_str());
	if (directory == NULL)
		return string();

	string htmlCode =
"<html>\
<head>\
<title>Index of " + request.getStartLine().requestTarget + "</title>\
</head>\
<body>\
<h1>Index of " + request.getStartLine().requestTarget + "</h1>\
<hr>\
<pre>";
	struct dirent *dp;
	while ((dp = readdir(directory)) != NULL)
	{
		char *fileName = dp->d_name;
		if (std::strncmp(fileName, ".", 2) == 0)
			continue;
		if (std::strncmp(fileName, "..", 3) == 0)
			continue;
		string pathName = fileName;
		if (dp->d_type == DT_DIR)
			pathName += '/';
		string line = "<a href=\"" + pathName + "\">" + pathName + "</a>";
		htmlCode += line + "<br>";
	}
	htmlCode +=
"</pre>\
</hr>\
</body>\
</html>";
	if (closedir(directory) == -1)
		std::perror("closedir");
	return htmlCode;
}

bool Utils::isDirectory(char const pathname[])
{
	struct stat s;
	if (stat(pathname, &s) == -1)
		return false;

	return !!(s.st_mode & S_IFDIR);
}

namespace
{
	template<typename T> string numToString(T number)
	{
		stringstream ss;
		ss << number;
		return ss.str();
	}
	bool isValidPathToDelete(string const &path)
	{
		t_vecStrings components = split(path, '/');
		for (t_vecStrings::const_iterator it = components.begin(); it != components.end(); ++it)
		{
			if (*it == ".." || *it == ".")
				return false;
		}
		return true;
	}
	t_vecStrings split(string const &str, char sep)
	{
		t_vecStrings output;
		std::size_t pos = 0;
		while (true)
		{
			std::size_t end = str.find(sep, pos);
			if (end == string::npos)
				break;
			if (end != pos)
				output.push_back(str.substr(pos, end - pos));
			pos = end + 1;
		}
		string temp = str.substr(pos);
		if (!temp.empty())
			output.push_back(temp);
		return output;
	}
}
