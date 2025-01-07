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
	e_phase parse(std::string buffer);

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
	class MessageBody
	{
	  public:
		MessageBody();
		MessageBody(MessageBody const &src);
		~MessageBody();

		MessageBody &operator=(MessageBody const &rhs);

		std::string data;
		std::size_t len;
		bool chunkCompleted;
	};
	enum e_statusFunction
	{
		STATUS_FUNCTION_NONE,
		STATUS_FUNCTION_SHOULD_RETURN,
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
	int parseHeader(std::string buffer);

	e_statusFunction readChunkSize();
	e_statusFunction readChunkData();
	e_statusFunction readTrailerFields();

	/*
	 * This function reads a line (without HTTP_DELIMITER "\r\n")
	 * and returns a pair of key-value (field-name and field-value).
	 * If an error occurs, it returns a pair of key-value that are empties.
	 */
	t_pairStrings parseFieldLine(std::string const &line);
	t_pairStrings parseStartLine(std::string const &line);

	e_statusFunction readBodyContent(char const contentLength[]);
};

#endif
