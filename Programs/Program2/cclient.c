/******************************************************************************
* cclient.c
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
#include <ctype.h>

#include "cclient.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "pollLib.h"
#include "signal.h"



#define DEBUG_FLAG 1

int main(int argc, char * argv[]) {
	int socketNum = 0;         //socket descriptor
	checkArgs(argc, argv);
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	initialPacket(socketNum, (uint8_t*) argv[1]);
	clientControl(socketNum, (uint8_t*) argv[1], strlen(argv[1]));
	return 0;
}

void sendInitialPacket(int mainServerSocket, uint8_t *handleName) {
	uint8_t dataBuffer[MAXBUF] = {};
	uint8_t msgLen;
	if(strlen((char*) handleName) > 100) {
		printf("Invalid handle, handle longer than 100 characters: %s\n", handleName);
		exit(-1);
	}
	if( ((handleName[0] - '0') >= 0) && (handleName[0] - '0') <= 9 ) {
		printf("Invalid handle, handle starts with a number\n");
		exit(-1);
	}
	msgLen = packFlagAndHandle(dataBuffer, strlen((char*) handleName), handleName, 1);
	sendToServer(mainServerSocket, dataBuffer, msgLen);
}

void recvInitialPacket(int mainServerSocket, uint8_t *handleName) {
	uint8_t flag = 0;

	verifyRecv(mainServerSocket, &flag, 1);

	
	if(flag == 2) {
		printf("Handle name \"%s\" has been accepted by the server!\n", handleName);
	}
	else if(flag == 3) {
		printf("Handle name \"%s\" has already been taken. Please try again with a different handle name\n", handleName);
		exit(-1);
	}
}

void initialPacket(int mainServerSocket, uint8_t *handleName) {
	sendInitialPacket(mainServerSocket, handleName);
	recvInitialPacket(mainServerSocket, handleName);	
}

void clientControl(int mainServerSocket, uint8_t *handleName, uint8_t handleLen) {

	int pollReturn;
	setupPollSet();
	addToPollSet(STDIN_FILENO);
	addToPollSet(mainServerSocket);
	
	printf("$: ");
	fflush(stdout);
	while(1) {
		pollReturn = pollCall(-1);

		// Error while Polling, or Timed Out
		if(pollReturn == -1) {
			perror("Timeout Error while Polling\n");
			exit(-1);
		}

		// Message from Server, unpack the packet
		if(pollReturn == mainServerSocket) {		
			unpackPacket(mainServerSocket);
			printf("\n$: ");
			fflush(stdout);
		}

		// From STDIN, send out a packet
		else {				
			packPacket(mainServerSocket, handleName, handleLen);
			printf("$: ");
			fflush(stdout);
		}
		
	}

}

/*
* Creates a Payload from STDIN data
*/
void packPacket(int mainServerSocket, uint8_t *handleName, uint8_t handleLen) {
	uint16_t stdinLen;
	uint8_t stdinBuf[MAX_STDIN];

	stdinLen = readFromStdin(stdinBuf);									// read from STDIN
	if(stdinLen > 1400) {
		printf("Total number of input characters cannot exceed 1400!\n");
		return;
	}
	processStdin(stdinBuf, stdinLen, handleLen, handleName, mainServerSocket);			// Process STDIN data
}

uint8_t getHandleName(uint8_t *inputData, uint8_t *destHandle) {

	uint8_t len = 0;
	while(!isspace(inputData[len]) && (inputData[len] != '\0')) {	
		destHandle[len] = inputData[len];
		len++;
	}

	// indicates the end of the stdin buffer: only matters for multicast, not enough handle names inputted
	if( (len == 0) && (inputData[len] == '\0')) {
		return 0;
	}
	destHandle[len] = '\0';
	return len;
}

uint16_t getDestHandles(uint8_t *handleBuf, uint8_t *stdinBuf, uint8_t numHandles) {
	uint8_t destHandleLen;
	uint16_t totLen = 0;
	uint8_t destHandle[MAX_HANDLE];

	memcpy(handleBuf, &numHandles, 1);
	totLen+=1;
	handleBuf+=1;

	
	for(uint8_t i = 0; i < numHandles; i++) {
		if(*(stdinBuf-1) == '\0') {
			printf("Not enough handles listed\n");
			return 0;
		}
		

		destHandleLen = getHandleName(stdinBuf, destHandle);	// grabs the Handle name
		
		memcpy(handleBuf, &destHandleLen, 1);					// add handle length
		memcpy(handleBuf+1, destHandle, destHandleLen);			// add handle name
		

		handleBuf += (destHandleLen + 1);						// increments output data pointer with len + handle name
		totLen += (destHandleLen + 1);							// increments total length	
		stdinBuf += (destHandleLen+1);							// increments stdin buffer with handle len + space
	}

	return totLen;
}

uint16_t processStdin(uint8_t *stdinBuf, uint16_t stdinLen, uint8_t srcHandleLen, uint8_t *srcHandleName, uint8_t socket) {
	char command;
	uint16_t pckDataLen = 0;
	uint8_t numDestHandles = 1;
	uint8_t flag;
	uint8_t broadcastFlag = 0;
	uint8_t spaceCount = 0;

	// foregoes the % sign in the STDIN buffer
	if(*stdinBuf++ != '%') {
		printf("Invalid command format\n");
		return 0;
	}
	stdinLen--;
	command = *stdinBuf++;
	

	while(*stdinBuf == ' ') {
		stdinBuf++;
		stdinLen -= 1;
	}
	// stdinLen -= spaceCount;
	switch(tolower(command)) {

		case 'b':
		case 'c':
		case 'm':
			// foregoes the command and space in STDIN buffer
			// stdinBuf += 2;
			// stdinLen -= 2;
			if(command == 'b') {
				flag = 4;
				numDestHandles = 0;
				broadcastFlag = 1;
			}
			if(command == 'c') {
				flag = 6;
				numDestHandles = stdinBuf[0] - '0'; 
				if((numDestHandles < 2) || (numDestHandles > 9)) {
					printf("The number of destination handles for %%C must be between 2 and 9\n");
					return 1;
				}
				stdinBuf+=2;
				stdinLen-=2;
			}

			if(command == 'm')  {
				flag = 5;
				numDestHandles = 1;
			}
			pckDataLen = packMessagePacket(stdinBuf, stdinLen, numDestHandles, srcHandleLen, srcHandleName, flag, socket, broadcastFlag);
			break;
		case 'l':
			flag = 10;
			sendPDU(socket, &flag, 1);
			break;
		case 'e':
			flag = 8;
			sendPDU(socket, &flag, 1);
			break;
		default:
			printf("Invalid command\n");
			return 0;
			break;
		
	}
	return pckDataLen;
	
	
}

uint16_t packMessagePacket(uint8_t *stdinBuf, uint16_t stdinLen, uint8_t numDestHandles, uint8_t srcHandleLen, uint8_t *srcHandleName, uint8_t flag, uint8_t socket, uint8_t broadcast) {
	uint16_t destHandleLen;
	uint16_t pckDataLen = 0;

	uint8_t payload[MAXBUF];
	uint8_t *payloadPtr = payload;
	

	pckDataLen += packFlagAndHandle(payloadPtr, srcHandleLen, srcHandleName, flag);	// Append Source Handle data and flag
	
	payloadPtr += pckDataLen;
	if(!broadcast) {
		destHandleLen = getDestHandles(payloadPtr, stdinBuf, numDestHandles);
		if(destHandleLen == 0) return 0;

		payloadPtr += destHandleLen;
		pckDataLen += destHandleLen;			// keep track of total packet length	
		stdinLen -= destHandleLen -1;				// remaining length is data length
		stdinBuf += destHandleLen - 1;
	}
	fragmentMsg(stdinLen, stdinBuf, payloadPtr, payload, pckDataLen, socket);

	return pckDataLen;
}

void fragmentMsg(uint16_t msgLen, uint8_t *msg, uint8_t *pckMsg, uint8_t *payload, uint16_t payloadLen, int8_t socket) {
	while(msgLen > 200) {
		memcpy(pckMsg, msg, MAX_TEXT - 1);
		pckMsg[MAX_TEXT-1] = '\0';
		
		sendPDU(socket, payload, payloadLen + MAX_TEXT);
		msg += 199;
		msgLen -= 199;
	}
	
	
	memcpy(pckMsg, msg, msgLen);
	pckMsg[msgLen] = '\0';
	if(msgLen == 0) {
		msgLen++;
	}
	sendPDU(socket, payload, payloadLen + msgLen);
	
}

void unpackMessagePacket(uint8_t *inputBuf, uint8_t broadcast) {
	uint8_t srcHandleLen;
	uint8_t srcHandle[MAX_HANDLE];
	uint8_t destHandleLen;
	uint8_t numDestHandle = 1;

	srcHandleLen = unpackPacketHandle(inputBuf++, srcHandle);

	inputBuf += srcHandleLen;					// increment input pointer

	if(!broadcast) {
		numDestHandle = *inputBuf++;				// get the number of destination clients
		// loop through all the destination clients
		while(numDestHandle != 0) {
			memcpy(&destHandleLen, inputBuf, 1);	// length of destination client
			inputBuf += 1 + destHandleLen;			// increment input pointer (length byte + handle length)
			numDestHandle -= 1;						// decrement number of handles
		}
	}
	
	printf("%s: %s", srcHandle, inputBuf);	// print the message from the source client!
	
}

void processServerPacket(int mainServerSocket, uint16_t msgLen, uint8_t *inputBuf) {
	uint8_t flag = inputBuf[0];
	uint8_t destHandle[MAX_HANDLE];
	uint8_t broadcastFlag = 0;
	
	printf("\n");
	switch(flag) {
		case 4:	// broadcast		
		case 5:	// message
		case 6:	// multicast
			if(flag == 4) broadcastFlag = 1;
			unpackMessagePacket(++inputBuf, broadcastFlag);
			break;
		case 7:
			unpackPacketHandle(&inputBuf[1], destHandle);
			printf("Client with handle %s does not exist", destHandle);
			break;
		case 9:
			removeFromPollSet(mainServerSocket);
			removeFromPollSet(STDIN_FILENO);
			exit(-1);
			break;

		case 11:
			listClients(mainServerSocket, msgLen, inputBuf);
			break;
	}	

	
}

void listClients(int mainServerSocket, uint16_t msgLen, uint8_t *inputBuf) {
	uint32_t hostNumClients, netNumClients;
	uint8_t handle[MAX_HANDLE];
	uint8_t handleLen;
	uint8_t flag = *inputBuf++;
	
	memcpy(&netNumClients, inputBuf, 4);
	inputBuf+=4;
	hostNumClients = ntohl(netNumClients);
	printf("Number of clients: %d", hostNumClients);

	msgLen = verifyRecv(mainServerSocket, inputBuf, MAXBUF);

	flag = *inputBuf++;
	while(flag != 13) {
		
		handleLen = *inputBuf;
		memcpy(handle, inputBuf+1, handleLen);
		handle[handleLen] = '\0';
		printf("\n\t%s", handle);
		msgLen = verifyRecv(mainServerSocket, inputBuf, MAXBUF);
		flag = *inputBuf++;
	}

}

/*
* checks to see if the server has terminated
*/
int verifyRecv(int mainServerSocket, uint8_t *dataBuffer, int bufferSize) {
	uint16_t messageLen = 0;
	
	messageLen = recvPDU(mainServerSocket, dataBuffer, MAXBUF);

	if (messageLen < 0) {		// Error on recv
		perror("recv call error\n");
		exit(-1);
	}

	if (messageLen > 0) {		// Got a Packet from the server
		return messageLen;
		
	}
	else {						// Server has closed
		closeClient(mainServerSocket);
		return 0;
	}

}

void unpackPacket(int mainServerSocket) {
	uint8_t dataBuffer[MAXBUF];
	uint16_t messageLen = 0;

	messageLen = verifyRecv(mainServerSocket, dataBuffer, MAXBUF);
	processServerPacket(mainServerSocket, messageLen, dataBuffer);
	
}

void closeClient(int mainServerSocket) {
	printf("\nServer Terminated\n");
	removeFromPollSet(mainServerSocket);
	removeFromPollSet(STDIN_FILENO);
	close(mainServerSocket);
	exit(-1);
}

void sendToServer(int socketNum, uint8_t *sendBuf, uint8_t sendLen) {

	int sent = 0;            	//actual amount of data sent/* get the data and send it   */
	
	sent =  sendPDU(socketNum, sendBuf, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	
}

uint16_t readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	uint16_t inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';

	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle server-name server-port \n", argv[0]);
		exit(1);
	}
}

