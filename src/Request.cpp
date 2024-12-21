#include <cerrno>
#include <cstdlib>

#include "Request.hpp"

using std::string;

// --- Public --- //
Request::Request()
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
	return IO_SUCCESS;
}

Request::e_phase Request::parse()
{
	if (_phase == PHASE_EMPTY)
	{
		// Parse the start line
	}
	if (_phase == PHASE_START_LINE)
	{
		// Parse the headers line
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
void Request::parseBody()
{
	t_mapString::const_iterator contentHeaderIt = _headers.find("CONTENT-LENGTH"),
								transferHeaderIt = _headers.find("TRANSFER-ENCODING");

	if (transferHeaderIt != _headers.end())
	{
	}
	else if (contentHeaderIt != _headers.end())
	{
		char *pEnd = NULL;
		unsigned long contentLen = std::strtoul(contentHeaderIt->second.c_str(), &pEnd, 10);
		if (*pEnd != '\0' || errno == ERANGE)
		{
			_statusCode = BAD_REQUEST;
			_phase = PHASE_ERROR;
			return;
		}
		unsigned long appendSize = contentLen - _body.size();
		if (_buffer.size() < appendSize)
		{
			_body += _buffer;
			string().swap(_buffer);
		}
		else
		{
			_body += _buffer.substr(0, appendSize);
			_buffer = _buffer.substr(appendSize);
			_phase = PHASE_BODY;
		}
	}
	else
		_phase = PHASE_BODY;
}
