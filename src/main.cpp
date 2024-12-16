/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sumseo <sumseo@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/07 16:00:23 by pnguyen-          #+#    #+#             */
/*   Updated: 2024/12/12 16:37:04 by sumseo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <exception>
#include <iostream>

#include "WebServer.hpp"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <iostream>

#define BUF_SIZE 500
#define PORT "8080"

using std::cerr;
using std::cout;
using std::endl;


void sigchld_handler(int s)
{
	(void)s;
	int saved_errno = errno;
	while(waitpid(-1,NULL,WNOHANG) > 0);
	errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
	if(sa->sa_family == AF_INET)
		return &(((struct sockaddr_in *)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int createServer(void)
{
	int socketFd;
	int newFd;
	struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; 
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; //use my IP address

	rv = getaddrinfo(NULL,PORT, &hints, &servinfo);
	if(rv!=0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
	}

	for(p = servinfo; p !=NULL; p= p->ai_next
	){
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

	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigactioin");
		exit(1);
	}
	printf("server: waiting for connections...\n");
	while(1)
	{
		sin_size = sizeof their_addr;
		newFd = accept(socketFd, (struct sockaddr *)&their_addr, &sin_size);
		if(newFd == -1)
		{
			perror("accept problem");
			continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
		printf("server : got connection from %s \n", s);
		char buffer[1024];

		ssize_t bytes_received = recv(newFd, buffer, sizeof(buffer), 0);
		if (bytes_received < 0) 
			std::cerr << "Error in recv: " << strerror(errno) << std::endl;
		else if (bytes_received == 0)
			std::cout << "Connection closed by peer" << std::endl;
		else 
		{
			std::cout << "Received " << bytes_received << " bytes" << std::endl;
			std::cout << "Data: " << std::string(buffer, bytes_received) << std::endl;
		}
			close(newFd);
	}
	return 0;
}


int	main(int argc, char **argv)
{
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " <configuration file>" << endl;
		return (1);
	}
	try
	{
		WebServer	server(argv[1]);
		createServer();
	}
	catch (const std::exception &e)
	{
		cerr << "Error: " << e.what() << endl;
		return (1);
	}


	return (0);
}
