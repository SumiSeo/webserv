#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include "WebServer.hpp"

using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecString;

namespace
{
	char const *const	SERVER_KEYS[] = {
		"listen",
		"server_name",
		"error_page",
		NULL,
	};
	char const *const	LOCATION_KEYS[] = {
		"root",
		"autoindex",
		NULL,
	};
}

namespace Utils
{
	t_vecString	split(const string &str, char delim);
	bool		isValidKeyServer(string const &key);
	bool		isValidKeyLocation(string const &key);
};

WebServer::WebServer()
{
	Server server;

}

WebServer::WebServer(const WebServer &src)
{
	if (this != &src)
	{
		*this = src;
	}
}

WebServer::WebServer(const char fileName[])
{
	t_vecString listTokens = readFile(fileName);
	searchTokens(listTokens);
	createServer();

}

WebServer::~WebServer()
{
}

WebServer &WebServer::operator=(const WebServer &rhs)
{
	if (this != &rhs)
	{
		*this = rhs;
	}
	return (*this);
}

// --- Private methods ---

WebServer::t_vecString WebServer::readFile(const char filename[])
{
	t_vecString	listTokens;

	ifstream infile(filename);
	infile.exceptions(ifstream::badbit);
	for (string content; std::getline(infile, content);)
	{
		content = content.substr(0, content.find('#'));
		stringstream stream(content);
		while (stream >> content)
		{
			t_vecString tokens(Utils::split(content, ';'));
			for (t_vecString::const_iterator it(tokens.begin()); it != tokens.end(); ++it)
				listTokens.push_back(*it);
		}
	}
	return (listTokens);
}

void WebServer::searchTokens(const t_vecString &tokens)
{
	string const server = "server";

	for (t_vecString::const_iterator it = tokens.begin(); it != tokens.end(); it++)
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

void WebServer::parseTokens(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
{
	if (start == end || *start != "{")
		throw std::runtime_error("Missing token '{'");

	start++;
	int depthCheck = 1;
	for (t_vecString::const_iterator it = start; it != end; it++)
	{
		if (*it == "server" || *it == "{" || *it == "}")
			continue ;
		t_vecString::const_iterator restart = it;
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
			break ;
	}

}

void WebServer::tokenToMap(t_vecString::const_iterator start,
	t_vecString::const_iterator end)
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
			t_vecString locationValues;
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
		t_vecString values;
		for (t_vecString::const_iterator it = start; it != end;)
		{
			++it;
			if (it != end)
				values.push_back(*it);
		}
		server._configs.insert(std::make_pair(*start, values));
	}
}

void WebServer::printKeyValues(void)
{

	for(vector<Server>::const_iterator serverIt = _servers.begin(); serverIt != _servers.end(); serverIt++)
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

void WebServer::loop(int socketFd)
{
	printf("server: waiting for connections...\n");
	int epollFd;
	struct epoll_event event, events[MAX_EVENTS];
	epollFd = epoll_create1(0);
	
	if(epollFd == -1)
	{
		std::cout<<"Failed to create an epoll instance"<<std::endl;
		close(socketFd);
	}

	event.events = EPOLLIN;
	event.data.fd = socketFd;
	if(epoll_ctl(epollFd, EPOLL_CTL_ADD, socketFd, &event) == -1)
	{
		std::cerr<<"Failed to add socket to epoll instance "<<std::endl;
		close(socketFd);
		close(epollFd);
		return;
	}

	std::cout<<"Server started listening on port : " << PORT << std::endl;
	while(true)
	{
		int numEvents = epoll_wait(epollFd, events, MAX_EVENTS, -1);
		if(numEvents == -1)
		{
			std::cerr << "Failed to wait for events: " << strerror(errno) << std::endl;
			std::cerr<<"Failed to wait for events"<<std::endl;
			break;
		}

		for(int i = 0; i < numEvents; ++i)
		{
			if(events[i].data.fd == socketFd)
			{
				struct sockaddr_in clientAddress;
				socklen_t clientAddressLength = sizeof(clientAddress);
				int clientFd = accept(socketFd, (struct sockaddr *)&clientAddress, &clientAddressLength);
				if(clientFd == -1)
				{
					std::cerr<<"Failed to accept client connection" <<std::endl;
					continue;
				}
				//Add client socket to epoll
				event.events = EPOLLIN;
				event.data.fd = clientFd;
				if(epoll_ctl(epollFd, EPOLL_CTL_ADD,clientFd, &event) == -1)
				{
					std::cerr << "Failed to add client socket to epoll instance." << std::endl;
					close(clientFd);
					continue;
				}
				handleClient(clientFd);
			}
			else
				{
					int clientFd = events[i].data.fd;
					handleClient(clientFd);
				}
		}
		
	}
	close(socketFd);
    close(epollFd);
}
int WebServer::createServer(void)
{
	int socketFd;
	struct addrinfo hints, *servinfo, *p;
	int yes = 1;
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	rv = getaddrinfo(NULL,PORT, &hints, &servinfo);
	if(rv!=0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
	}

	for(p = servinfo; p !=NULL; p= p->ai_next)
	{
		socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(socketFd == -1)
		{
			perror("server: socket");
			continue;
		}
		if(setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		if(bind(socketFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(socketFd);
			perror("server: bind");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo);
	if(p == NULL)
	{
		fprintf(stderr, "server : failed to bind");
		exit(1);
	}
	if(listen(socketFd, 10) == -1)
	{
		perror("listen");
		exit(1);
	}
	loop(socketFd);
	return 0;
}

//---Static functions-- -

t_vecString Utils::split(const string &str, char delim)
{
	t_vecString	words;

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
	return (words);
}

bool	Utils::isValidKeyServer(string const &key)
{
	for (std::size_t i = 0; SERVER_KEYS[i] != NULL; ++i)
	{
		if (key == SERVER_KEYS[i])
			return true;
	}
	return false;
}

bool	Utils::isValidKeyLocation(string const &key)
{
	for (std::size_t i = 0; LOCATION_KEYS[i] != NULL; ++i)
	{
		if (key == LOCATION_KEYS[i])
			return true;
	}
	return false;
}


// static function
void WebServer::sigchld_handler(int s)
{
	(void)s;
	int saved_errno = errno;
	while(waitpid(-1,NULL,WNOHANG) > 0);
	errno = saved_errno;
}