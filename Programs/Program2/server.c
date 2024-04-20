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

#define MAXBUF 1400
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void addNewSocket(int mainServerSocket);
void processClient(int clientSocket, uint8_t msgLen, uint8_t *dataBuf);
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
			recvFromClient(pollSocket);
		}
	}
}

void addNewSocket(int mainServerSocket) {
	int clientSocket;
	clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

void processClient(int clientSocket, uint8_t msgLen, uint8_t *dataBuf) {
	uint8_t *tempBuf = dataBuf;
	uint8_t flag = dataBuf[0];
	uint8_t srcHandleLen = dataBuf[1];
	uint8_t srcHandle[101];
	uint8_t destHandleLen;
	uint8_t destHandle[101];
	uint8_t numDestHandle = 1;
	uint32_t destSocket = 4;

	uint8_t sendBuf[MAXBUF] = {};


	uint8_t messageLen = 0;
	

	memcpy(srcHandle, &dataBuf[2], srcHandleLen);
	srcHandle[srcHandleLen] = '\0';
	

	switch(flag) {
		case 1:

			if(addHandle(srcHandle, srcHandleLen, clientSocket)) {		// handle name exists
				dataBuf[2] = 3;
				
				sendPDU(clientSocket, dataBuf, 1);
			}
			else {
				dataBuf[2] = 2;
				sendPDU(clientSocket, dataBuf, 1);
			}
			break;

		case 5:
			tempBuf += srcHandleLen + 2;
			numDestHandle = tempBuf[0];
			
			tempBuf += 1;
			while(numDestHandle != 0) {
				memcpy(&destHandleLen, tempBuf, 1);
				memcpy(destHandle, tempBuf+1, destHandleLen);
				destHandle[destHandleLen] = '\0';
				if((destSocket = getHandleToSocket(destHandle, destHandleLen)) > 0) {	// handle exists
					memcpy(sendBuf + 2, dataBuf, msgLen);

					// WORK HERE TO LIMIT
					sendPDU(destSocket, sendBuf, msgLen);
				}
				else {
					dataBuf[2] = 7;
					memcpy(dataBuf + 3, &destHandleLen, 1);
					memcpy(dataBuf + 4, destHandle, destHandleLen);
					sendPDU(clientSocket, dataBuf, 2 + destHandleLen);
				}
				numDestHandle -= 1;
			}

	}
		
}

void getDestHandles() {

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
		processClient(clientSocket, messageLen, dataBuffer);
		// printf("\nMessage received on Socket %d, length: %d Data: %s\n", clientSocket, messageLen, dataBuffer);

		// sendConfToClient(clientSocket, messageLen);

	}
	else
	{
		printf("Connection closed by client socket %d\n", clientSocket);
		removeFromPollSet(clientSocket);
		removeHandle(clientSocket);
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

