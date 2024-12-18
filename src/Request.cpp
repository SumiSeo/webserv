#include <sys/socket.h>

#include "Request.hpp"

namespace
{
	std::size_t const MAX_BUFFER_LENGTH = 8192;
};

// --- Public --- //
Request::Request(Request const &src):
	_fd(src._fd),
	_phase(src._phase),
	_buffer(src._buffer),
	_startLine(src._startLine),
	_headers(src._headers),
	_body(src._body)
{
}

Request::Request(int fd):
	_fd(fd),
	_phase(PHASE_EMPTY)
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
	{
	}
	if (_phase == PHASE_START_LINE)
	{
	}
	if (_phase == PHASE_HEADERS)
	{
	}
	if (_phase == PHASE_BODY)
	{
	}
	return _phase;
}

// --- Private --- //
Request::Request()
{
}
