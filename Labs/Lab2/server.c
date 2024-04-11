/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Modified by Chris Bae
* Use at your own risk.  
*
*****************************************************************************/

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
#include <stdint.h>

#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

int recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void addNewSocket(int mainServerSocket);
int processClient(int pollReturn);


int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	int pollReturn = 0;
	int numBytes;

	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	setupPollSet();
	addToPollSet(mainServerSocket);

	while(1) {
		pollReturn = pollCall(-1);
		if(pollReturn == -1) {
			perror("Timeout Error while Polling\n");
			exit(-1);
		}

		if(pollReturn == mainServerSocket) {
			// wait for client to connect
			addNewSocket(mainServerSocket);
		}
		else {
			numBytes = processClient(pollReturn);
		}
	}
	
	/* close the sockets */
	close(clientSocket);
	close(mainServerSocket);

	
	return 0;
}

void addNewSocket(int mainServerSocket) {
	int clientSocket;
	clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

int processClient(int pollReturn) {
	return recvFromClient(pollReturn);
}

int recvFromClient(int clientSocket)
{
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket
	//if ((messageLen = safeRecv(clientSocket, dataBuffer, MAXBUF, 0)) < 0)
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{
		printf("Message received, length: %d Data: %s\n", messageLen, dataBuffer);
		return messageLen;
	}
	else
	{
		printf("Connection closed by other side\n");
		removeFromPollSet(clientSocket);
		return 0;
	}
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

