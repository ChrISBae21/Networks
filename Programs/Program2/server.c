/******************************************************************************
* server.c
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

#include "server.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "pollLib.h"
#include "handleTable.h"


#define DEBUG_FLAG 1



int main(int argc, char *argv[]) {
	uint32_t mainServerSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	portNumber = checkArgs(argc, argv);
	//initializes the handle table
	setupHandleTable(mainServerSocket);
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);
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

void processClient(int clientSocket, uint8_t inputBufLen, uint8_t *inputBuf) {
	uint8_t srcHandle[MAX_HANDLE];
	
	uint8_t flag = inputBuf[0];
	uint8_t srcHandleLen = inputBuf[1];

	switch(flag) {
		case 1:
			memcpy(srcHandle, &inputBuf[2], srcHandleLen);
			srcHandle[srcHandleLen] = '\0';
			if(addHandle(srcHandle, srcHandleLen, clientSocket)) {		// handle name exists
				inputBuf[0] = 3;
				
				sendPDU(clientSocket, inputBuf, 1);
				removeFromPollSet(clientSocket);
				close(clientSocket);

			}
			else {
				inputBuf[0] = 2;
				sendPDU(clientSocket, inputBuf, 1);
			}
			break;
		case 4:
			sendBroadcast(clientSocket, inputBufLen, inputBuf);
			break;
			//
		case 5:
			
		case 6:
			unpackMessagePacket(clientSocket, inputBufLen, inputBuf);
			break;
		case 8:
			removeHandle(clientSocket);
			inputBuf[0] = 9;
			sendPDU(clientSocket, inputBuf, 1);
			break;

		case 10:
			sendClientList(clientSocket, inputBuf);
			break;


	}
		
}


void sendClientList(int clientSocket, uint8_t *sendBuf) {
	uint32_t hostNumClients, netNumClients;
	uint32_t i = 0;
	uint8_t handleLen;
	uint8_t handleName[MAX_HANDLE];
	uint16_t totLen;

	hostNumClients = getNumClients();
	netNumClients = htonl(hostNumClients);
	sendBuf[0] = 11;
	memcpy(sendBuf+1, &netNumClients, 4);
	sendPDU(clientSocket, sendBuf, 5);

	while(hostNumClients != 0) {
		handleLen = getSocketToHandle(i, handleName);
		if(handleLen > 0) {
			totLen = packFlagAndHandle(sendBuf, handleLen, handleName, 12);
			sendPDU(clientSocket, sendBuf, totLen);
			hostNumClients--;
		}
		i++;
	}
	sendBuf[0] = 13;
	sendPDU(clientSocket, sendBuf, 1);

}


void sendBroadcast(int clientSocket, uint8_t sendLen, uint8_t *sendBuf) {
	uint32_t hostNumClients;
	uint32_t i = 0;
	uint8_t handleLen;
	uint8_t handleName[MAX_HANDLE];


	hostNumClients = getNumClients() - 1;
	while(hostNumClients != 0) {

		if(i != clientSocket) {
			handleLen = getSocketToHandle(i, handleName);
			if(handleLen > 0) {
				sendPDU(i, sendBuf, sendLen);
				hostNumClients--;
			}
		}
		i++;
	}

}



// returns the number of valid handles
// outHandles is organized as: one byte handle len, handle name,...
void unpackMessagePacket(int srcClientSocket, uint8_t inputBufLen, uint8_t *inputBuf) {
	uint8_t destHandle[MAX_HANDLE];
	uint8_t numHandles, destHandleLen;
	uint8_t sendBuf[MAXBUF];
	uint8_t *packetHandleBuf;
	int destSocket;

	// skip the flag, source len, and source handle bytes
	packetHandleBuf = inputBuf + inputBuf[1] + 2;
	numHandles = *packetHandleBuf++;

	for(int i = 0; i < numHandles; i++) {
		
		destHandleLen = unpackPacketHandle(packetHandleBuf, destHandle);

		// is a valid handle
		if((destSocket = getHandleToSocket(destHandle, destHandleLen)) > 0) {

			sendPDU(destSocket, inputBuf, inputBufLen);
			printf("Input size: %d", inputBufLen);
		}

		// destination handle doesn't exist
		else {
			packFlagAndHandle(sendBuf, destHandleLen, destHandle, 7);
			sendPDU(srcClientSocket, sendBuf, 2 + destHandleLen);
		}
		packetHandleBuf += 1 + destHandleLen;
	}
	

}


void recvFromClient(int clientSocket) {
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);
	if (messageLen < 0) {
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0) {
		processClient(clientSocket, messageLen, dataBuffer);

	}
	else {
		printf("Connection closed by client socket %d\n", clientSocket);
		removeHandle(clientSocket);
		removeFromPollSet(clientSocket);
		close(clientSocket);
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

