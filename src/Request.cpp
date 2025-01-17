
#include <sys/socket.h>
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iostream>
#include "Request.hpp"

using std::string;
using std::stringstream;

namespace
{
  std::size_t const MAX_BUFFER_LENGTH = 8192;
	char const HTTP_DELIMITER[] = "\r\n";
	char const HTTP_WHITESPACES[] = "\t ";
}

namespace Utils
{
	string uppercaseString(string const &input);
	int toupper(char c);
	bool isValidToken(string const &input);
	string trimString(string const &input, string const &charset);
	bool isValidFieldValue(string const &fieldValue);
}

// --- Public --- //
Request::Request():
	_fd(-1),
	_phase(PHASE_ERROR),
	_statusCode(NONE)
{
}

Request::Request(Request const &src):
	_fd(src._fd),
	_phase(src._phase),
	_buffer(src._buffer),
	_startLine(src._startLine),
	_headers(src._headers),
	_body(src._body),
	_statusCode(src._statusCode)
{
}

Request::Request(int fd):
	_fd(fd),
	_phase(PHASE_EMPTY),
	_statusCode(NONE)
{
}

Request::~Request()
{
}

Request &Request::operator=(Request const &rhs)
{
	_fd = rhs._fd;
	_phase = rhs._phase;
	_buffer = rhs._buffer;
	_startLine = rhs._startLine;
	_headers = rhs._headers;
	_body = rhs._body;
	_statusCode = rhs._statusCode;
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
		parseBody();
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

int Request::getFd()
{
	return _fd;
}


// --- Private --- //

void Request::parseHeader()
{
	std::string tmp;
	std::size_t temp = 0;

	while (temp < _buffer.size()) {	
		while (temp < _buffer.size() && (_buffer[temp] == '\r' || _buffer[temp] == '\n'))
			++temp;
		std::size_t end = _buffer.find("\r\n", temp);
		if (end == std::string::npos)
			break; 
   		 std::string line = _buffer.substr(temp, end - temp);
   		 t_pairStrings field = parseFieldLine(line);
    	_headers.insert(field);
   		_buffer.erase(temp, (end - temp) + 2); 
}

	/* Debugging: These line belows will be deleted in the end */
	printStartLine();
	printRequest();
	_phase = PHASE_HEADERS;
}

void Request::parseStartLine()
{
	size_t start = _buffer.find("\n");
	std::string line;
	if (start != std::string::npos)
	{
		 std::string tmpLine(_buffer.begin(), _buffer.begin() + start);
		_buffer.erase(_buffer.begin(), _buffer.begin() + start + 1); 
		line = tmpLine;
	}
	size_t targetPos = line.find("/");
	size_t targetPosEnd = line.find("H");
	_startLine.method = line.substr(0,targetPos - 1);
	_startLine.requestTarget = line.substr(targetPos, targetPosEnd  - targetPos - 1 );
	_startLine.httpVersion = line.substr(targetPosEnd);
	_phase = PHASE_START_LINE;
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
	t_mapString::iterator contentHeaderIt = _headers.find("CONTENT-LENGTH"),
							transferHeaderIt = _headers.find("TRANSFER-ENCODING");

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
	_headers["CONTENT-LENGTH"] = number.str();
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
		_buffer = _buffer.substr(appendSize);
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
