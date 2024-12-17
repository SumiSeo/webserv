#ifndef __REQUEST_HPP__
# define __REQUEST_HPP__

# include <string>
# include <map>

class Request
{
  public:
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

	Request(Request const &src);
	Request(int fd);
	~Request();

	Request	&operator=(Request const &rhs);

	e_IOReturn retrieve();
	e_phase parse();

  protected:

  private:
	struct s_startLine
	{
		std::string method;
		std::string requestTarget;
		std::string httpVersion;
	};

	typedef std::map<std::string, std::string> t_mapString;

	int _fd;
	e_phase _phase;
	std::string _buffer;
	s_startLine _startLine;
	t_mapString _headers;
	std::string _body;

	Request();
};

#endif
