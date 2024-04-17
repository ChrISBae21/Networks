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
#include "handleTable.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void addNewSocket(int mainServerSocket);
void processClient(int pollSocket);
void serverControl(int mainServerSocket);
void sendConfToClient(int socketNum, int recMsgLen);

int main(int argc, char *argv[])
{
	uint32_t mainServerSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	//initializes the handle table
	setupHandleTable(mainServerSocket);

	//start doing server things
	serverControl(mainServerSocket);

	/* close the sockets */
	close(clientSocket);
	close(mainServerSocket);
	return 0;
}


void serverControl(int mainServerSocket) {
	int pollSocket;

	setupPollSet();
	addToPollSet(mainServerSocket);
	while(1) {
		pollSocket = pollCall(-1);
		if(pollSocket == -1) {
			perror("Timeout Error while Polling\n");
			exit(-1);
		}

		if(pollSocket == mainServerSocket) {
			// wait for client to connect
			addNewSocket(mainServerSocket);
		}
		else {
			processClient(pollSocket);
		}
	}
}

void addNewSocket(int mainServerSocket) {
	int clientSocket;
	clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

void processClient(int pollSocket) {
	recvFromClient(pollSocket);
}

void recvFromClient(int clientSocket)
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
		printf("\nMessage received on Socket %d, length: %d Data: %s\n", clientSocket, messageLen, dataBuffer);

		sendConfToClient(clientSocket, messageLen);

	}
	else
	{
		printf("Connection closed by client socket %d\n", clientSocket);
		removeFromPollSet(clientSocket);
		close(clientSocket);
	}
}


void sendConfToClient(int socketNum, int recMsgLen)
{
	// uint8_t sendBuf[MAXBUF];   //data buffer
	// int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	uint8_t confMsg[] = {'M', 'e', 's', 's', 'a', 'g', 'e', ' ', 'R', 'e', 'c', 'e', 'i', 'v', 'e', 'd', '\0'};
	int sendLen = 17;
	
	// sendLen = readFromStdin(sendBuf);
	printf("Sending Confirmation to Client...\n");
	
	//sent =  safeSend(socketNum, sendBuf, sendLen, 0);
	sent =  sendPDU(socketNum, confMsg, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("Sent Confirmation\n");
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

