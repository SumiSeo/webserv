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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUF_SIZE 500

using std::cerr;
using std::cout;
using std::endl;

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
	}
	catch (const std::exception &e)
	{
		cerr << "Error: " << e.what() << endl;
		return (1);
	}
	int status;
	    char ipstr[INET6_ADDRSTRLEN];
	struct addrinfo hints;
	struct addrinfo *p;
	struct addrinfo *servinfo; // will point to the results (getaddrinfo)
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	status = getaddrinfo("www.example.net", NULL, &hints, &servinfo);
	printf("STATUS %d\n",status);
	if(status!=0)
	{
		//there is an error
    	fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));	
		return 1;	
	}
	if (servinfo != NULL) {
    	printf("Address resolved successfully.\n");
	}
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		printf("yo\n");
		void *addr;
		const char *ipver;

		if(p->ai_family == AF_INET)
		{
			struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else
		{
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
		}
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);	
	}

	freeaddrinfo(servinfo);
	return (0);
}
