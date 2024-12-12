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
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	ssize_t nread;
	char buf[BUF_SIZE];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; //Allo Ipv4 or IPv6;
	hints.ai_socktype = SOCK_DGRAM; // datagram socket;
	hints.ai_flags = AI_PASSIVE; // For wildcard Ip address;
	hints.ai_protocol = 0; //For any protocol
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(NULL, argv[1], &hints, &result);
	if(s!=0)
		exit(0);
	//get addr info returns a list of address structures
	//try each address until we successfullly bind;
	// if socket fails to bind, we close the socket and try the next address
	for(rp = result; rp!=NULL; rp = rp->ai_next){
		sfd= socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sfd==-1)
			continue;
		if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0 )
			break; // it measn successful
		close(sfd);
	}

	freeaddrinfo(result);
	if(rp == NULL)
	{
		exit(0);
	}
	for(;;){
		peer_addr_len = sizeof(peer_addr);
		nread = recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
		if(nread == -1)
			continue; // ignore failed request
		char host[NI_MAXHOST], service[NI_MAXSERV];
		s = getnameinfo((struct sockaddr *) &peer_addr, peer_addr_len, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
		if(s ==0)
			printf("Recived %zd bytes from %s:%s\n", nread, host, service);
		else
			printf("getname info %s\n", gai_strerror(s));
		if(sendto(sfd, buf, nread, 0, (struct sockaddr *) &peer_addr, peer_addr_len) !=nread)
			printf("error sending response");
	}
	
	return (0);
}
