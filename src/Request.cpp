#include "Request.hpp"

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
	return IO_SUCCESS;
}

Request::e_phase Request::parse()
{
	return _phase;
}

// --- Private --- //
Request::Request()
{
}
