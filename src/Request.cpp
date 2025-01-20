#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <sys/socket.h>

#include "Request.hpp"

using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecStrings;

namespace
{
	std::size_t const MAX_BUFFER_LENGTH = 8192;
	char const HTTP_DELIMITER[] = "\r\n";
	char const HTTP_WHITESPACES[] = "\t ";
	char const *VALID_METHODS[] = {
		"GET",
		"POST",
		"DELETE",
		NULL
	};

	std::size_t parseSize(string const &size);
}

namespace Utils
{
	string uppercaseString(string const &input);
	int toupper(char c);
	bool isValidToken(string const &input);
	string trimString(string const &input, string const &charset);
	bool isValidFieldValue(string const &fieldValue);
	bool isValidMethod(string const &method);
}

// --- Public --- //
Request::Request():
	_fd(-1),
	_socketFd(-1),
	_phase(PHASE_ERROR),
	_statusCode(NONE),
	_servers(NULL),
	_serverIndex(0),
	_maxBodySize(0)
{
}

Request::Request(Request const &src):
	_fd(src._fd),
	_socketFd(src._socketFd),
	_phase(src._phase),
	_buffer(src._buffer),
	_startLine(src._startLine),
	_headers(src._headers),
	_body(src._body),
	_statusCode(src._statusCode),
	_servers(src._servers),
	_locationKey(src._locationKey),
	_serverIndex(src._serverIndex),
	_address(src._address),
	_port(src._port),
	_maxBodySize(src._maxBodySize)
{
}

Request::Request(int fd, int socketFd, t_vecServers *servers):
	_fd(fd),
	_socketFd(socketFd),
	_phase(PHASE_EMPTY),
	_statusCode(NONE),
	_servers(servers),
	_serverIndex(0),
	_maxBodySize(0)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if (getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != -1)
	{
		char ipStr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
		_address = ipStr;
		stringstream number;
		number << ntohs(addr.sin_port);
		_port = number.str();
	}
}

Request::~Request()
{
}

Request &Request::operator=(Request const &rhs)
{
	_fd = rhs._fd;
	_socketFd = rhs._socketFd;
	_phase = rhs._phase;
	_buffer = rhs._buffer;
	_startLine = rhs._startLine;
	_headers = rhs._headers;
	_body = rhs._body;
	_statusCode = rhs._statusCode;
	_servers = rhs._servers;
	_locationKey = rhs._locationKey;
	_serverIndex = rhs._serverIndex;
	_address = rhs._address;
	_port = rhs._port;
	_maxBodySize = rhs._maxBodySize;
	return *this;
}

Request::e_IOReturn Request::retrieve()
{
	char buffer[MAX_BUFFER_LENGTH + 1];
	ssize_t bytesRead = recv(_fd, buffer, MAX_BUFFER_LENGTH, 0);

	if (bytesRead == -1)
		return IO_ERROR;
	if (bytesRead == 0)
		return IO_DISCONNECT;

	buffer[bytesRead] = '\0';
	_buffer += buffer;
	return IO_SUCCESS;
}

Request::e_phase Request::parse()
{
	if (_phase == PHASE_EMPTY)
		parseStartLine();
	if (_phase == PHASE_START_LINE)
		parseHeader();
	if (_phase == PHASE_HEADERS)
	{
		if (_headers.find("HOST") == _headers.end())
		{
			_phase = PHASE_ERROR;
			_statusCode = BAD_REQUEST;
		}
		else
		{
			filterServers();
			if (verifyRequest() == STATUS_FUNCTION_NONE)
				parseBody();
		}
	}
	if (_phase == PHASE_BODY)
		_phase = PHASE_COMPLETE;
	if (_phase != PHASE_COMPLETE && _phase != PHASE_ERROR)
	{
		if (_buffer.size() >= MAX_BUFFER_LENGTH)
		{
			_phase = PHASE_ERROR;
			_statusCode = BAD_REQUEST;
		}
	}
	return _phase;
}

Request::StartLine const &Request::getStartLine() const
{
	return _startLine;
}

Request::t_mapString const &Request::getHeaders() const
{
	return _headers;
}

std::string const &Request::getBody() const
{
	return _body.data;
}

e_statusCode Request::getStatusCode() const
{
	return _statusCode;
}

Request::e_phase Request::getPhase() const
{
	return _phase;
} 

int Request::getFd() const
{
	return _fd;
}

string const &Request::getAddress() const
{
	return _address;
}

string const &Request::getPort() const
{
	return _port;
}

std::size_t Request::getServerIndex() const
{
	return _serverIndex;
}

string Request::getLocationKey() const
{
	return _locationKey;
}

void Request::reset()
{
	_phase = PHASE_EMPTY;
	_statusCode = NONE;
}

// --- Private --- //

void Request::parseHeader()
{
	std::string tmp;
	std::size_t temp = 0;

	while (temp < _buffer.size()) {
		while (temp < _buffer.size() && (_buffer[temp] == '\r' || _buffer[temp] == '\n'))
			++temp;
		std::size_t end = _buffer.find("\r\n");
		if (end == 0)
		{
			_buffer.erase(0, std::strlen(HTTP_DELIMITER));
			break;
		}
		if (end == std::string::npos)
			break; 
   		 std::string line = _buffer.substr(temp, end - temp);
   		 t_pairStrings field = parseFieldLine(line);
    	_headers.insert(field);
		_buffer.erase(temp, (end - temp) + std::strlen(HTTP_DELIMITER));
}

	/* Debugging: These line belows will be deleted in the end */
	printStartLine();
	printRequest();
	_phase = PHASE_HEADERS;
}

void Request::parseStartLine()
{
	size_t start = _buffer.find("\r\n");
	if (start != std::string::npos)
	{
		std::string line(_buffer.begin(), _buffer.begin() + start);
		_buffer.erase(_buffer.begin(), _buffer.begin() + start + std::strlen(HTTP_DELIMITER));
		size_t targetPos = line.find(" ");
		_startLine.method = line.substr(0,targetPos);
		size_t targetPosEnd = line.find(" ", targetPos + 1);
		if (!Utils::isValidMethod(_startLine.method) || targetPosEnd == string::npos)
		{
			_phase = PHASE_ERROR;
			_statusCode = BAD_REQUEST;
			return;
		}
		_startLine.requestTarget = line.substr(targetPos + 1, targetPosEnd  - targetPos - 1 );
		if (_startLine.requestTarget[0] != '/')
		{
			_phase = PHASE_ERROR;
			_statusCode = BAD_REQUEST;
			return;
		}
		_startLine.httpVersion = line.substr(targetPosEnd + 1);
		if (_startLine.httpVersion != "HTTP/1.1")
		{
			_phase = PHASE_ERROR;
			_statusCode = BAD_REQUEST;
			return;
		}
		_phase = PHASE_START_LINE;
	}
}

Request::MessageBody::MessageBody():
	len(0),
	chunkCompleted(false)
{
}

Request::MessageBody::MessageBody(MessageBody const &src):
	data(src.data),
	len(src.len),
	chunkCompleted(src.chunkCompleted)
{
}

Request::MessageBody::~MessageBody()
{
}

Request::MessageBody &Request::MessageBody::operator=(MessageBody const &rhs)
{
	data = rhs.data;
	len = rhs.len;
	chunkCompleted = rhs.chunkCompleted;
	return *this;
}


void Request::parseBody()
{
	t_mapString::iterator contentHeaderIt = _headers.find("CONTENT_LENGTH"),
							transferHeaderIt = _headers.find("TRANSFER_ENCODING");

	if (transferHeaderIt != _headers.end())
	{
		if (!_body.chunkCompleted)
		{
			if (_body.data.size() == _body.len)
			{
				if (readChunkSize() == STATUS_FUNCTION_SHOULD_RETURN)
					return;
			}
			if (readChunkData() == STATUS_FUNCTION_SHOULD_RETURN)
				return;
		}

		if (readTrailerFields() == STATUS_FUNCTION_SHOULD_RETURN)
			return;

		_buffer = _buffer.substr(std::strlen(HTTP_DELIMITER));
		_headers.erase(transferHeaderIt);
		_phase = PHASE_BODY;
	}
	else if (contentHeaderIt != _headers.end())
	{
		if (readBodyContent(contentHeaderIt->second.c_str()) == STATUS_FUNCTION_SHOULD_RETURN)
			return;
	}
	else
		_phase = PHASE_BODY;
	printBodyMessage();
}

void Request::filterServers()
{
	string host = _headers["HOST"].substr(0, _headers["HOST"].find(":"));
	t_vecServers const &servers = *_servers;
	std::size_t index = 0;
	for (t_vecServers::const_iterator it = servers.begin(); it != servers.end(); ++it, ++index)
	{
		if (!it->checkValuesOf("server_name", host))
			continue;

		{
			struct sockaddr_in addr;
			socklen_t len = sizeof(addr);
			if (getsockname(_socketFd, reinterpret_cast<sockaddr*>(&addr), &len) == -1)
				continue;

			char ipStr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &addr.sin_addr, ipStr, sizeof(ipStr));
			string address = ipStr;
			stringstream number;
			number << ntohs(addr.sin_port);
			string port = number.str();

			string listenAddress = "127.0.0.1";
			string listenPort = "8080";
			string listenValue = it->getValueOf("listen");
			if (!listenValue.empty())
			{
				std::size_t pos = listenValue.rfind(":");
				listenAddress = listenValue.substr(0, pos);
				listenPort = listenValue.substr(pos + 1);
			}

			if (listenAddress != address || listenPort != port)
				continue;
		}
		_serverIndex = index;
		break;
	}
	Server const &server = (*_servers)[_serverIndex];
	_locationKey = server.searchLocationKey(_startLine.requestTarget);
}

Request::e_statusFunction Request::verifyRequest()
{
	Server const &server = (*_servers)[_serverIndex];
	Location const &locationBlock = server._locations.at(_locationKey);
	if (!locationBlock.getValuesOf("allow_methods").empty())
	{
		if (!locationBlock.checkValuesOf("allow_methods", _startLine.method))
		{
			_phase = PHASE_ERROR;
			_statusCode = METHOD_NOT_ALLOWED;
			return STATUS_FUNCTION_SHOULD_RETURN;
		}
	}
	else
	{
		if (!server.checkValuesOf("allow_methods", _startLine.method))
		{
			_phase = PHASE_ERROR;
			_statusCode = METHOD_NOT_ALLOWED;
			return STATUS_FUNCTION_SHOULD_RETURN;
		}
	}
	string maxBodySize = locationBlock.getValueOf("client_max_body_size");
	if (maxBodySize.empty())
		maxBodySize = server.getValueOf("client_max_body_size");
	if (maxBodySize.empty())
		maxBodySize = "1M";
	_maxBodySize = parseSize(maxBodySize);
	return STATUS_FUNCTION_NONE;
}

Request::e_statusFunction Request::readChunkSize()
{
	long contentLen = 0;
	std::size_t pos = _buffer.find(HTTP_DELIMITER);
	if (pos == string::npos)
		return STATUS_FUNCTION_SHOULD_RETURN;

	contentLen = std::strtol(_buffer.c_str(), NULL, 16);
	if (contentLen < 0 || errno == ERANGE)
	{
		_statusCode = BAD_REQUEST;
		_phase = PHASE_ERROR;
		return STATUS_FUNCTION_SHOULD_RETURN;
	}

	_buffer = _buffer.substr(pos + std::strlen(HTTP_DELIMITER));
	_body.len += static_cast<std::size_t>(contentLen);
	if (_body.len > _maxBodySize)
	{
		_statusCode = CONTENT_TOO_LARGE;
		_phase = PHASE_ERROR;
		return STATUS_FUNCTION_SHOULD_RETURN;
	}

	return STATUS_FUNCTION_NONE;
}

Request::e_statusFunction Request::readChunkData()
{
	while (_body.data.size() < _body.len)
	{
		std::size_t appendSize = std::min(_body.len - _body.data.size(), _buffer.size());
		_body.data += _buffer.substr(0, appendSize);

		_buffer = _buffer.substr(appendSize);
		std::size_t pos = _buffer.find(HTTP_DELIMITER);
		if (pos == string::npos)
			return STATUS_FUNCTION_SHOULD_RETURN;

		_buffer = _buffer.substr(pos + std::strlen(HTTP_DELIMITER));

		if (readChunkSize() == STATUS_FUNCTION_SHOULD_RETURN)
			return STATUS_FUNCTION_SHOULD_RETURN;
	}
	_body.len = _body.data.size();
	stringstream number;
	number << _body.len;
	_headers["CONTENT_LENGTH"] = number.str();
	_body.chunkCompleted = true;
	return STATUS_FUNCTION_NONE;
}

Request::e_statusFunction Request::readTrailerFields()
{
	std::size_t pos = _buffer.find(HTTP_DELIMITER);
	if (pos == 0)
		return STATUS_FUNCTION_NONE;

	while (true)
	{
		if (pos == string::npos)
			return STATUS_FUNCTION_SHOULD_RETURN;

		string fieldLine = _buffer.substr(0, pos);
		t_pairStrings field = parseFieldLine(fieldLine);

		if (!field.first.empty())
		{
			if (_headers.find(field.first) != _headers.end())
				_headers[field.first] += ", " + field.second;
			else
				_headers.insert(field);
		}

		_buffer = _buffer.substr(pos + std::strlen(HTTP_DELIMITER));
		pos = _buffer.find(HTTP_DELIMITER);

		if (pos == 0) // If there is two HTTP_DELIMITER in a row, then the body message is finished
			break;
	}
	return STATUS_FUNCTION_NONE;
}




Request::t_pairStrings Request::parseFieldLine(string const &line)
{
	t_pairStrings field;
	std::size_t pos = line.find(':');
	if (pos == string::npos)
		return field;

	string fieldName = line.substr(0, pos);
	if (!Utils::isValidToken(fieldName))
		return field;
	string fieldValue = Utils::trimString(line.substr(pos + 1), HTTP_WHITESPACES);
	if (!Utils::isValidFieldValue(fieldValue))
		return field;
	field.first = Utils::uppercaseString(fieldName);
	field.second = fieldValue;
	return field;
}

Request::e_statusFunction Request::readBodyContent(char const contentLength[])
{
	char *pEnd = NULL;
	long contentLen = std::strtol(contentLength, &pEnd, 10);
	if (*pEnd != '\0' || contentLen < 0 || errno == ERANGE)
	{
		_statusCode = BAD_REQUEST;
		_phase = PHASE_ERROR;
		return STATUS_FUNCTION_SHOULD_RETURN;
	}

	if (static_cast<std::size_t>(contentLen) > _maxBodySize)
	{
		_statusCode = CONTENT_TOO_LARGE;
		_phase = PHASE_ERROR;
		return STATUS_FUNCTION_SHOULD_RETURN;
	}

	std::size_t appendSize = static_cast<std::size_t>(contentLen) - _body.len;
	if (_buffer.size() < appendSize)
	{
		_body.data += _buffer;
		_body.len += _buffer.size();
		string().swap(_buffer);
	}
	else
	{
		_body.data += _buffer.substr(0, appendSize);
		_body.len += appendSize;
		_body.chunkCompleted = true;
		_buffer = _buffer.substr(appendSize + std::strlen(HTTP_DELIMITER));
		_phase = PHASE_BODY;
	}
	return STATUS_FUNCTION_NONE;
}

// --- Static Functions --- //
string Utils::uppercaseString(string const &input)
{
	string output(input);
	std::transform(output.begin(), output.end(), output.begin(), &Utils::toupper);
	return output;
}

int Utils::toupper(char c)
{
	if (c == '-')
		return '_';
	return std::toupper(static_cast<unsigned char>(c));
}

bool Utils::isValidToken(string const &input)
{
	if (input.size() < 1)
		return false;
	std::size_t const HASH_MAP_SIZE = 256;
	bool hashMap[HASH_MAP_SIZE];
	std::memset(hashMap, false, sizeof(*hashMap) * HASH_MAP_SIZE);
	hashMap[static_cast<unsigned char>('!')] = 1;
	hashMap[static_cast<unsigned char>('#')] = 1;
	hashMap[static_cast<unsigned char>('$')] = 1;
	hashMap[static_cast<unsigned char>('%')] = 1;
	hashMap[static_cast<unsigned char>('&')] = 1;
	hashMap[static_cast<unsigned char>('\'')] = 1;
	hashMap[static_cast<unsigned char>('*')] = 1;
	hashMap[static_cast<unsigned char>('+')] = 1;
	hashMap[static_cast<unsigned char>('-')] = 1;
	hashMap[static_cast<unsigned char>('.')] = 1;
	hashMap[static_cast<unsigned char>('^')] = 1;
	hashMap[static_cast<unsigned char>('_')] = 1;
	hashMap[static_cast<unsigned char>('`')] = 1;
	hashMap[static_cast<unsigned char>('|')] = 1;
	hashMap[static_cast<unsigned char>('~')] = 1;

	for (string::const_iterator it = input.begin(); it != input.end(); ++it)
	{
		if (!std::isalpha(static_cast<unsigned char>(*it))
		&& !std::isdigit(static_cast<unsigned char>(*it))
		&& !hashMap[static_cast<unsigned char>(*it)])
			return false;
	}
	return true;
}

string Utils::trimString(string const &input, string const &charset)
{
	std::size_t start = input.find_first_not_of(charset);
	if (start == string::npos)
		start = 0;
	std::size_t end = input.find_last_not_of(charset);
	return input.substr(start, end - start + 1);
}

bool Utils::isValidFieldValue(string const &fieldValue)
{
	if (fieldValue.empty())
		return true;

	string::const_iterator it = fieldValue.begin();
	if (!std::isprint(static_cast<unsigned char>(*it))
	&& !(static_cast<unsigned char>(*it) >= 0x80 && static_cast<unsigned char>(*it) <= 0xFF))
		return false;

	++it;
	for (; it != fieldValue.end(); ++it)
	{
		unsigned char const &c = *it;
		if (c != '\t'
		&& c != ' '
		&& !std::isprint(c)
		&& !(c >= 0x80 && c <= 0xFF))
			return false;
	}
	return true;
}

bool Utils::isValidMethod(string const &method)
{
	for (std::size_t i = 0; VALID_METHODS[i] != NULL; ++i)
	{
		if (method == VALID_METHODS[i])
			return true;
	}
	return false;
}

namespace
{
	std::size_t parseSize(string const &size)
	{
		char *pEnd = NULL;
		long num = std::strtol(size.c_str(), &pEnd, 10);
		if (errno == ERANGE || num < 0)
			return 0;

		long mult = 0;
		switch (std::toupper(*pEnd))
		{
			case 'B':
				mult = 1l;
				break;
			case 'K':
				mult = 1000l;
				break;
			case 'M':
				mult = 1000l * 1000l;
				break;
			case 'G':
				mult = 1000 * 1000l * 1000l;
				break;
			default:
				mult = 0;
				break;
		}
		if (mult == 0)
			return 0;

		unsigned long res = std::numeric_limits<unsigned long>::max() / static_cast<unsigned long>(mult);
		if (static_cast<unsigned long>(num) > res)
			return 0;

		return static_cast<std::size_t>(num) * static_cast<std::size_t>(mult);
	}
}

// -- debugging fuction --//
void Request::printRequest()
{
	for(t_mapString::iterator it = _headers.begin(); it != _headers.end(); it++)
		std::cout<<"key : " << it->first <<" " << "value : " << it->second <<std::endl;
}

void Request::printStartLine()
{
	std::cout<<"START LINE method : "<< _startLine.method <<std::endl;
	std::cout<<"START LINE request Target: "<< _startLine.requestTarget<<std::endl;
	std::cout<<"START LINE http version: "<< _startLine.httpVersion<<std::endl;
}

void Request::printBodyMessage()
{
	std::cout << "Body Message : [" << _body.data << ']' << std::endl;
}
