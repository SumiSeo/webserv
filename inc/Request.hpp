#ifndef __REQUEST_HPP__
# define __REQUEST_HPP__

# include <string>
# include <map>

# include "statusCode.hpp"

class Request
{
  public:
	/* New Variable Types */
	enum e_IOReturn
	{
		IO_ERROR,
		IO_DISCONNECT,
		IO_SUCCESS,
	};
	enum e_phase
	{
		PHASE_EMPTY,
		PHASE_START_LINE,
		PHASE_HEADERS,
		PHASE_BODY,
		PHASE_COMPLETE,
		PHASE_ERROR,
	};

	/* Methods */
	Request();
	Request(Request const &src);
	Request(int fd);
	~Request();

	Request	&operator=(Request const &rhs);

	// -- Main Functions -- //
	e_IOReturn retrieve();
	e_phase parse();

  protected:

  private:
	/* Typedefs */
	typedef std::map<std::string, std::string> t_mapString;

	/* New Variable Types */
	struct StartLine
	{
		std::string method;
		std::string requestTarget;
		std::string httpVersion;
	};

	/* Members */
	int _fd;
	e_phase _phase;
	std::string _buffer;
	StartLine _startLine;
	t_mapString _headers;
	std::string _body;
	e_statusCode _statusCode;

	/* Methods */
	// -- Utils Functions -- //
	void parseBody();
};

#endif
