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
	_phase(PHASE_EMPTY),
	_statusCode(NONE)
{
	std::cout<<"*****************TESTREQUEST***********"<<std::endl;
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
	_phase(PHASE_ERROR),
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
	return IO_SUCCESS;
}

Request::e_phase Request::parse(std::string buffer)
{
	
	if(buffer[0] == '\0')
		_phase = PHASE_EMPTY;
	else
		_phase = PHASE_START_LINE;
	if (_phase == PHASE_START_LINE)
	{
		int startLineIndex= parseHeader(buffer);
		std::cout<<"&&&&&"<< startLineIndex<<std::endl;

	}
	if (_phase == PHASE_HEADERS)
	{
		// Parse the body line
		parseBody();
	}
	if (_phase == PHASE_BODY)
	{
		// Set complete
		_phase = PHASE_COMPLETE;
	}
	return _phase;
}



// --- Private --- //

int Request::parseHeader(std::string buffer)
{
	std::cout<<"******Parse header called*****"<<std::endl;
	int start;
	start = 0;
	
	while(buffer[start]!='\n')
	{
		start++;
	}
	char line[start];
	int i = 0;
	while(i < start)
	{
		line[i] = buffer[i];
		i++;
	}
	std::string f_line = line;
	std::cout<<"start"<<start << f_line<<std::endl;

	t_pairStrings field = parseFieldLine(f_line);
	std::cout << "**->" << "field.first : " << field.first<< "<-**" <<std::endl;
	std::cout << "**->" << "field.second :" << field.second<< "<-**" <<std::endl;
	return i;
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
	{	
		std::cout<<"field none1" << std::endl;
		return field;
	}
	string fieldValue = Utils::trimString(line.substr(pos + 1), HTTP_WHITESPACES);
	if (!Utils::isValidFieldValue(fieldValue))
	{				
		std::cout<<"field none2" << std::endl;
		return field;
	}

	field.first = Utils::uppercaseString(fieldName);
	field.second = fieldValue;
	std::cout<<"field first"<<field.first << std::endl;
	std::cout<<"fied second"<<field.second<<std::endl;
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
