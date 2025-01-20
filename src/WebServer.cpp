#include <fcntl.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include "WebServer.hpp"
#include "Request.hpp"
#include "Response.hpp"

using std::cerr;
using std::endl;
using std::ifstream;
using std::map;
using std::set;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecStrings;

namespace
{
	char const *const SERVER_KEYS[] = {
		"listen",
		"root",
		"server_name",
		"autoindex",
		"error_page",
		"client_max_body_size",
		"allow_methods",
		"index",
		"return",
		NULL,
	};
	char const *const LOCATION_KEYS[] = {
		"root",
		"autoindex",
		"error_page",
		"client_max_body_size",
		"allow_methods",
		"index",
		"return",
		"cgi",
		"upload_path",
		NULL,
	};
	char const *const DEFAULT_PORT = "8080";
	std::size_t const MAX_EVENTS = 512;
}

namespace Utils
{
	void sigchld_handler(int s);
	t_vecStrings split(string const &str, char delim);
	bool isValidKeyServer(string const &key);
	bool isValidKeyLocation(string const &key);
	string getListenAddress(t_vecStrings const &listenValue);
	string getListenPort(t_vecStrings const &listenValue);
};

// --- Public --- //
WebServer::WebServer(char const fileName[]):
	_epollFd(-1)
{
	t_vecStrings listTokens = readFile(fileName);
	searchTokens(listTokens);
	if (_servers.size() == 0 || _servers[0]._locations.size() == 0
		|| !isValidConfig())
		throw std::runtime_error("Not a valid config file");
	createServer();
	if (_servers.size() == 0)
		throw std::runtime_error("Cannot listen any server");
}

WebServer::~WebServer()
{
	std::for_each(_socketFds.begin(), _socketFds.end(), &close);
	if (_epollFd != -1)
		close(_epollFd);
	for (map<int, Request>::const_iterator it = _requests.begin(); it != _requests.end(); ++it)
		close(it->first);
}

// --- Private --- //
WebServer::WebServer()
{
}

WebServer::WebServer(WebServer const &):
	_epollFd(-1)
{
}

WebServer &WebServer::operator=(WebServer const &)
{
	return *this;
}

t_vecStrings WebServer::readFile(char const filename[])
{
	t_vecStrings listTokens;

	ifstream infile(filename);
	infile.exceptions(ifstream::badbit);
	if (!infile.is_open())
		throw std::runtime_error(string("Cannot open file ") + filename);
	for (string content; std::getline(infile, content);)
	{
		content = content.substr(0, content.find('#'));
		stringstream stream(content);
		while (stream >> content)
		{
			t_vecStrings tokens(Utils::split(content, ';'));
			for (t_vecStrings::const_iterator it(tokens.begin()); it != tokens.end(); ++it)
				listTokens.push_back(*it);
		}
	}
	return (listTokens);
}

void WebServer::searchTokens(t_vecStrings const &tokens)
{
	string const server = "server";

	for (t_vecStrings::const_iterator it = tokens.begin(); it != tokens.end(); it++)
	{
		string value = *it;
		if (value == server)
		{
			_servers.push_back(Server());
			parseTokens(it + 1, tokens.end());
		}
	}
	printKeyValues();
}

bool WebServer::isValidConfig() const
{
	typedef map<string, t_vecStrings> t_mapStringVecStrings;
	typedef map<string, Location> t_mapStringLoc;

	for (vector<Server>::const_iterator serverIt = _servers.begin(); serverIt != _servers.end(); ++serverIt)
	{
		for (t_mapStringVecStrings::const_iterator keyValueIt = serverIt->_configs.begin(); keyValueIt != serverIt->_configs.end(); ++keyValueIt)
		{
			t_vecStrings const &values = keyValueIt->second;
			if (values.empty())
				return false;

			string const &key = keyValueIt->first;
			if (!isValidValue(key, values))
				return false;
		}
		bool mainLocationFound = false;
		for (t_mapStringLoc::const_iterator locationIt = serverIt->_locations.begin(); locationIt != serverIt->_locations.end(); ++locationIt)
		{
			Location const &location = locationIt->second;
			for (t_mapStringVecStrings::const_iterator keyValueIt = location._pairs.begin(); keyValueIt != location._pairs.end(); ++keyValueIt)
			{
				t_vecStrings const &values = keyValueIt->second;
				if (values.empty())
					return false;

				string const &key = keyValueIt->first;
				if (!isValidValue(key, values))
					return false;
			}
			if (locationIt->first == "/")
				mainLocationFound = true;
		}
		if (!mainLocationFound)
			return false;
	}
	return true;
}

void WebServer::parseTokens(t_vecStrings::const_iterator start,
	t_vecStrings::const_iterator end)
{
	if (start == end || *start != "{")
		throw std::runtime_error("Missing token '{'");

	start++;
	int depthCheck = 1;
	for (t_vecStrings::const_iterator it = start; it != end; it++)
	{
		if (*it == "server" || *it == "{" || *it == "}")
			continue ;
		t_vecStrings::const_iterator restart = it;
		while (it != end)
		{
			if (*it == "{") 
			{
				++it;
				depthCheck = 0;
			} 
			else if (*it == ";" && depthCheck == 1) {
        		tokenToMap(restart, it);
        		restart = ++it; 
			} 
			else if (*it == "}") 
			{
				tokenToMap(restart, it);
				depthCheck = 1;
				break; 
			} 
			else 
				it++;
		}
		if (it == end)
			break;
	}
}

void WebServer::tokenToMap(t_vecStrings::const_iterator start,
	t_vecStrings::const_iterator end)
{
	Server &server = _servers.back();

	if (*start == "location")	
	{
		 while (start != end) 
		{
		
			++start;
			if (start == end) 
				break; 

			Location location;
			t_vecStrings locationValues;
			string locationKey = *start;
			++start;
			while (start != end && *start != "}") 
			{
				if (*start == "{") 
				{
					++start; 
					continue;
				}
				string key = *start;
				locationValues.clear();

				if (!Utils::isValidKeyLocation(key))
					throw std::runtime_error("Invalid key found in location block");

				++start; 
				while (start != end && *start != ";" && *start != "}") 
				{
					locationValues.push_back(*start);
					++start;
				}
				location._pairs.insert(std::make_pair(key, locationValues));
				if (start != end && *start == ";") 
					++start;
				
			}
			server._locations.insert(std::make_pair(locationKey, location));
    	}
	}
	else
	{
		if (!Utils::isValidKeyServer(*start))
			throw std::runtime_error("Invalid key found server block");
		t_vecStrings values;
		for (t_vecStrings::const_iterator it = start; it != end;)
		{
			++it;
			if (it != end)
				values.push_back(*it);
		}
		server._configs.insert(std::make_pair(*start, values));
	}
}

void WebServer::printKeyValues()
{
	for(std::vector<Server>::const_iterator serverIt = _servers.begin(); serverIt != _servers.end(); serverIt++)
	{
		std::cout << "Server" << std::endl;
		for(std::map<string, vector<string> >::const_iterator it = serverIt->_configs.begin(); it!=serverIt->_configs.end(); it++)
		{
			std::cout << "key [" << it->first << "]" <<std::endl;
			for (vector<string>::const_iterator valIt = it->second.begin(); valIt != it->second.end(); ++valIt)
				std::cout << "  value [" << *valIt << "]"<< std::endl;
		}	

		for (std::map<string, Location>::const_iterator it = serverIt->_locations.begin(); it != serverIt->_locations.end(); ++it) 
		{
			std::cout << "Location : key [" << it->first << "]" << std::endl;
			const Location& loc = it->second;
			for (std::map<string, vector<string> >::const_iterator pairIt = loc._pairs.begin(); 
				pairIt != loc._pairs.end(); 
				++pairIt) 
			{
				std::cout << "  pair key [" << pairIt->first << "]"<< std::endl;
				for (vector<string>::const_iterator valIt = pairIt->second.begin(); 
					valIt != pairIt->second.end(); 
					++valIt)
					std::cout << "  pair value [" << *valIt << "]"<< std::endl;
			}
		}
	}
}

void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void handleClient(int clientFd) 
{
	// char s[INET6_ADDRSTRLEN];
	// struct sockaddr_storage their_addr; 
	// inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

	// printf("server : got connection from %s \n", s);
	char buffer[1024];
	
	ssize_t bytes_received = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytes_received < 0) 
		std::cerr << "Error in recv: " << strerror(errno) << std::endl;
	else if (bytes_received == 0)
		std::cout << "Connection closed by peer" << std::endl;
	else 
	{
		std::cout << "Received " << bytes_received << " bytes" << std::endl;
		std::cout << "Data: " << std::string(buffer, bytes_received) << std::endl;
	}
}

void WebServer::loop()
{
	printf("server: waiting for connections...\n");
	struct epoll_event event, events[MAX_EVENTS];
	_epollFd = epoll_create1(0);
	if(_epollFd == -1)
	{
		std::perror("epoll_create1");
		return;
	}

	std::memset(&event, 0, sizeof(event));
	event.events = EPOLLIN;
	for (set<int>::const_iterator it = _socketFds.begin(); it != _socketFds.end(); ++it)
	{
		event.data.fd = *it;
		if(epoll_ctl(_epollFd, EPOLL_CTL_ADD, *it, &event) == -1)
		{
			std::perror("epoll_ctl(EPOLL_CTL_ADD)");
			return;
		}
	}

	while(true)
	{
		int numEvents = epoll_wait(_epollFd, events, MAX_EVENTS, -1);
		if(numEvents == -1)
		{
			std::perror("epoll_wait");
			break;
		}
		for(int i = 0; i < numEvents; ++i)
		{
			int fd = events[i].data.fd;
			bool shouldDisconnect = false;
			if (events[i].events & EPOLLERR)
			{
				cerr << "Error event occured on a fd" << endl;
				if (_socketFds.find(fd) != _socketFds.end())
					_socketFds.erase(fd);
				else
					_requests.erase(fd);
				if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
					std::perror("epoll_ctl(EPOLL_CTL_DEL)");
				close(fd);
				continue;
			}
			if (events[i].events & EPOLLHUP)
			{
				if (_socketFds.find(fd) != _socketFds.end())
					_socketFds.erase(fd);
				else if (_requests.find(fd) != _requests.end())
					_requests.erase(fd);
				else if (_cgiFdToResponseFd.find(fd) != _cgiFdToResponseFd.end())
					handleCGIInput(fd);
				if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
					std::perror("epoll_ctl(EPOLL_CTL_DEL)");
				close(fd);
				continue;
			}
			if (events[i].events & EPOLLIN)
			{
				if (_socketFds.find(fd) != _socketFds.end())
				{
					int clientFd = accept(fd, NULL, NULL);
					if(clientFd == -1)
					{
						std::perror("accept");
						continue;
					}
					if (fcntl(clientFd, F_SETFD, FD_CLOEXEC) == -1)
					{
						std::perror("fcntl");
						close(clientFd);
						continue;
					}
					std::memset(&event, 0, sizeof(event));
					event.events = EPOLLIN | EPOLLOUT;
					event.data.fd = clientFd;
					if(epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &event) == -1)
					{
						std::perror("epoll_ctl(EPOLL_CTL_ADD)");
						close(clientFd);
						continue;
					}
					_requests[clientFd] = Request(clientFd, fd, &_servers);
					continue;
				}
				else if (_requests.find(fd) != _requests.end())
				{
					Request &request = _requests[fd];

					Request::e_IOReturn recvReturn = request.retrieve();
					if (recvReturn == Request::IO_ERROR || recvReturn == Request::IO_DISCONNECT)
					{
						if (recvReturn == Request::IO_ERROR)
							std::perror("recv");
						shouldDisconnect = true;
					}
					else
					{
						Request::e_phase phase = request.parse();
						if (phase == Request::PHASE_ERROR || phase == Request::PHASE_COMPLETE)
						{
							_responses[fd] = Response(request, _servers[request.getServerIndex()]);
							Response &response = _responses[fd];
							int const &cgiFd = response.getFdCGI();
							if (cgiFd != -1)
							{
								std::memset(&event, 0, sizeof(event));
								event.events = EPOLLIN | EPOLLOUT;
								event.data.fd = cgiFd;
								if(epoll_ctl(_epollFd, EPOLL_CTL_ADD, cgiFd, &event) == -1)
								{
									std::perror("epoll_ctl(EPOLL_CTL_ADD)");
									close(cgiFd);
									response.setEndCGI();
								}
								else
									_cgiFdToResponseFd[cgiFd] = fd;
							}
							
							// TODO: Create a response for either of these 2 phases
							// we will create a map for response 
							// int fd of request 
							// key = int (fd), value = Response class
						}
					}
				}
				else if (_cgiFdToResponseFd.find(fd) != _cgiFdToResponseFd.end())
					shouldDisconnect = handleCGIInput(fd);
			}
			if (!shouldDisconnect && events[i].events & EPOLLOUT)
			{
				// TODO: Check if a response exists and if it is complete to send it
			}
			if (shouldDisconnect)
			{
				if(epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
					std::perror("epoll_ctl(EPOLL_CTL_DEL)");
				_requests.erase(fd);
				_cgiFdToResponseFd.erase(fd);
				close(fd);
			}
		}
	}
}

int WebServer::createServer()
{
	int socketFd;
	struct addrinfo hints, *servinfo, *p;
	int yes = 1;
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	for (vector<Server>::const_iterator it = _servers.begin(); it != _servers.end(); it++)
	{
		char const *port = DEFAULT_PORT,
					*address = NULL;
		string listenPort,
				listenAddress;
		if (it->_configs.find("listen") != it->_configs.end())
		{
			listenPort = Utils::getListenPort(it->_configs.at("listen"));
			listenAddress = Utils::getListenAddress(it->_configs.at("listen"));
			if (listenPort.empty() || listenAddress.empty())
				throw std::runtime_error("Invalid listen values");
			port = listenPort.c_str();
			address = listenAddress.c_str();
		}

		rv = getaddrinfo(address, port, &hints, &servinfo);

		if(rv!=0)
			throw std::runtime_error(gai_strerror(rv));

		for(p = servinfo; p !=NULL; p= p->ai_next)
		{
			socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

			if(socketFd == -1)
				continue;

			if(setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(int)) == -1) {
				close(socketFd);
				continue;
			}
			if(bind(socketFd, p->ai_addr, p->ai_addrlen) == -1)
			{
				close(socketFd);
				continue;
			}
			break;
		}
		freeaddrinfo(servinfo);
		if(p == NULL)
		{
			std::cerr << "server : failed to bind" << std::endl;
			continue;
		}
		if(listen(socketFd, 10) == -1)
		{
			std::perror("listen");
			close(socketFd);
			continue;
		}
		if (fcntl(socketFd, F_SETFD, FD_CLOEXEC) == -1)
		{
			std::perror("fcntl");
			close(socketFd);
			continue;
		}
		_socketFds.insert(socketFd);
	}
	return 0;
}

bool WebServer::handleCGIInput(int fd)
{
	int clientFd = _cgiFdToResponseFd[fd];
	Response &response = _responses[clientFd];
	Response::e_IOReturn recvReturn = response.retrieve();
	if (recvReturn == Response::IO_ERROR || recvReturn == Response::IO_DISCONNECT)
	{
		if (recvReturn == Response::IO_ERROR)
			std::perror("recv");
		response.setEndCGI();
		return true;
	}
	return false;
}

bool WebServer::isValidValue(std::string const &key, t_vecStrings const &values) const
{
	if (key == "root")
	{
		string const &value = values.at(0);
		return value.at(value.size() - 1) == '/';
	}
	else if (key == "autoindex")
	{
		string const &value = values.at(0);
		return value == "on" || value == "off";
	}
	else if (key == "error_page")
		return values.size() > 1;
	else if (key == "index")
	{
		string const &value = values.at(0);
		return value.at(value.size() - 1) != '/';
	}
	else if (key == "return")
		return values.size() > 1;
	else if (key == "upload_path")
	{
		string const &value = values.at(0);
		return value.at(value.size() - 1) == '/';
	}
	return true;
}
//////////** ---Static functions-- **///////////
t_vecStrings Utils::split(string const &str, char delim)
{
	t_vecStrings	words;

	std::size_t pos = 0;
	while (true)
	{
		std::size_t end = str.find(delim, pos);
		if (end == string::npos)
			break ;
		string	token = str.substr(pos, end - pos);
		if (!token.empty())
			words.push_back(token);
		words.push_back(str.substr(end, 1)); // add the delim
		pos = end + 1;
	}
	if (pos < str.size())
		words.push_back(str.substr(pos));
	return words;
}

bool Utils::isValidKeyServer(string const &key)
{
	for (std::size_t i = 0; SERVER_KEYS[i] != NULL; ++i)
	{
		if (key == SERVER_KEYS[i])
			return true;
	}
	return false;
}

bool Utils::isValidKeyLocation(string const &key)
{
	for (std::size_t i = 0; LOCATION_KEYS[i] != NULL; ++i)
	{
		if (key == LOCATION_KEYS[i])
			return true;
	}
	return false;
}

void Utils::sigchld_handler(int s)
{
	(void)s;
	int saved_errno = errno;
	while(waitpid(-1,NULL,WNOHANG) > 0);
	errno = saved_errno;
}

string Utils::getListenAddress(t_vecStrings const &listenValue)
{
	if (listenValue.size() == 0)
		return string();
	string const &value = listenValue[0];
	std::size_t pos = value.rfind(':');
	if (pos == string::npos)
		return string();
	return value.substr(0, pos);
}

string Utils::getListenPort(t_vecStrings const &listenValue)
{
	if (listenValue.size() == 0)
		return string();
	string const &value = listenValue[0];
	std::size_t pos = value.rfind(':');
	if (pos == string::npos)
		return string();
	return value.substr(pos + 1);
}
