#ifndef __REQUEST_HPP__
# define __REQUEST_HPP__

# include <map>
# include <string>
# include <utility>

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
	typedef std::pair<std::string, std::string> t_pairStrings;

	/* New Variable Types */
	struct StartLine
	{
		std::string method;
		std::string requestTarget;
		std::string httpVersion;
	};
	struct MessageBody
	{
		MessageBody();

		std::string data;
		std::size_t len;
		bool chunkCompleted;
	};

	/* Members */
	int _fd;
	e_phase _phase;
	std::string _buffer;
	StartLine _startLine;
	t_mapString _headers;
	MessageBody _body;
	e_statusCode _statusCode;

	/* Methods */
	// -- Utils Functions -- //
	void parseBody();

	/*
	 * This function reads a line (without HTTP_DELIMITER "\r\n")
	 * and returns a pair of key-value (field-name and field-value)
	 */
	t_pairStrings parseFieldLine(std::string const &line);
};

#endif
