
// Hugh Smith April 2017
// Network code to support TCP/UDP client and server connections

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "gethostbyname.h"


// This funciton creates a UDP socket on the server side and binds to that socket.  
// It prints out the port number and returns the socket number.

int udpServerSetup(int serverPort)
{
	struct sockaddr_in6 serverAddress;
	int socketNum = 0;
	int serverAddrLen = 0;	
	
	// create the socket
	if ((socketNum = socket(AF_INET6,SOCK_DGRAM,0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}
	
	// set up the socket
	memset(&serverAddress, 0, sizeof(struct sockaddr_in6));
	serverAddress.sin6_family = AF_INET6;    		// internet (IPv6 or IPv4) family
	serverAddress.sin6_addr = in6addr_any ;  		// use any local IP address
	serverAddress.sin6_port = htons(serverPort);   // if 0 = os picks 

	// bind the name (address) to a port
	if (bind(socketNum,(struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("bind() call error");
		exit(-1);
	}

	/* Get the port number */
	serverAddrLen = sizeof(serverAddress);
	getsockname(socketNum,(struct sockaddr *) &serverAddress,  (socklen_t *) &serverAddrLen);
	printf("Server using Port #: %d\n", ntohs(serverAddress.sin6_port));

	return socketNum;	
	
}

// This function opens a socket and fills in the serverAdress structure using the hostName and serverPort.  
// It assumes the address structure is created before calling this.
// Returns the socket number and the filled in serverAddress struct.

int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char * hostName, int serverPort)
{
	int socketNum = 0;
	char ipString[INET6_ADDRSTRLEN];
	uint8_t * ipAddress = NULL;
	
	// create the socket
	if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}
  	 	
	memset(serverAddress, 0, sizeof(struct sockaddr_in6));
	serverAddress->sin6_port = ntohs(serverPort);
	serverAddress->sin6_family = AF_INET6;	
	
	if ((ipAddress = gethostbyname6(hostName, serverAddress)) == NULL)
	{
		exit(-1);
	}
		
	
	inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
	printf("Server info - IP: %s Port: %d \n", ipString, serverPort);
		
	return socketNum;
}


