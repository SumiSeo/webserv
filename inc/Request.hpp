#ifndef __REQUEST_HPP__
# define __REQUEST_HPP__

# include <ctime>
# include <map>
# include <string>
# include <utility>
# include <vector>

# include "statusCode.hpp"
# include "WebServer.hpp"

class Request
{
  private:
	/* Typedefs */
	typedef std::vector<Server> t_vecServers;

  public:
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
	Request(int fd, int socketFd, t_vecServers *servers);
	~Request();

	Request	&operator=(Request const &rhs);

	// -- Main Functions -- //
	e_IOReturn retrieve();
	e_phase parse();

	StartLine const &getStartLine() const;
	t_mapString const &getHeaders() const;
	std::string const &getBody() const;
	e_statusCode getStatusCode() const;
	e_phase getPhase() const; 
	int getFd() const; 
	std::string const &getAddress() const;
	std::string const &getPort() const;
	std::size_t getServerIndex() const;
	std::string getLocationKey() const;
	std::time_t getTime() const;

	void reset();

  protected:

  private:
	/* New Variable Types */
	enum e_statusFunction
	{
		STATUS_FUNCTION_NONE,
		STATUS_FUNCTION_SHOULD_RETURN,
	};

	/* Members */
	int _fd;
	int _socketFd;
	e_phase _phase;
	std::string _buffer;
	StartLine _startLine;
	t_mapString _headers;
	MessageBody _body;
	e_statusCode _statusCode;
	t_vecServers *_servers;
	std::string _locationKey;
	int _serverIndex;
	std::string _address;
	std::string _port;
	std::size_t _maxBodySize;
	std::time_t _timeUpdated;


	/* Methods */
	// -- Utils Functions -- //
	void parseStartLine();
	void parseHeader();
	void parseBody();
	e_statusFunction filterServers();
	e_statusFunction verifyRequest();

	e_statusFunction readChunkSize();
	e_statusFunction readChunkData();
	e_statusFunction readTrailerFields();

	/*
	 * This function reads a line (without HTTP_DELIMITER "\r\n")
	 * and returns a pair of key-value (field-name and field-value).
	 * If an error occurs, it returns a pair of key-value that are empties.
	 */
	t_pairStrings parseFieldLine(std::string const &line);
	
	e_statusFunction readBodyContent(char const contentLength[]);

	// -- debugging function -- /
	void printRequest();
	void printStartLine();
	void printBodyMessage();

};

#endif
