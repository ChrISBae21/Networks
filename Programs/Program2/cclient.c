/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
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

#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "pollLib.h"
#include "signal.h"


#define MAXBUF 1400
#define DEBUG_FLAG 1

void sendToServer(int socketNum, uint8_t *sendBuf, uint8_t sendLen);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void clientControl(int mainServerSocket, uint8_t *handleName, uint8_t handleLen);
uint8_t processMsgFromServer(int mainServerSocket, uint8_t *retBuffer);
void initialPacket(int mainServerSocket, uint8_t *handleName);
uint32_t processStdin(uint8_t *pckDataBuf, uint8_t *stdinBuf, uint32_t stdinLen, uint8_t *flag);
uint8_t getDestHandles(uint8_t *handleBuf, uint8_t *stdinBuf, uint8_t numHandles);
uint8_t addSrcHandle(uint8_t *sendBuf, uint8_t handleLen, uint8_t *handleName, uint8_t flag);
void fromServer(int clientSocket, uint8_t msgLen, uint8_t *dataBuf);


int main(int argc, char * argv[]) {
	int socketNum = 0;         //socket descriptor
	checkArgs(argc, argv);
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	clientControl(socketNum, (uint8_t*) argv[1], strlen(argv[1]));
	return 0;
}



void initialPacket(int mainServerSocket, uint8_t *handleName) {
	uint8_t dataBuffer[MAXBUF] = {};
	uint8_t msgLen;
	uint8_t flag = 0;
	msgLen = addSrcHandle(dataBuffer, strlen((char*) handleName), handleName, 1);
	
	sendToServer(mainServerSocket, dataBuffer, msgLen);

	msgLen = processMsgFromServer(mainServerSocket, &flag);

	

	if(flag == 2) {
		printf("Handle name \"%s\" has been accepted by the server!\n", handleName);
	}
	else if(flag == 3) {
		printf("Handle name \"%s\" has already been taken. Please try again with a different handle name\n", handleName);
		kill(getpid(), 2);
	}
	
	// NEED TO RECIEVE THE PDU FROM THE SERVER AND VERIFY THE FLAG !!!!!!!!!!!!!!!!!!!!!!!!!!!
}

void clientControl(int mainServerSocket, uint8_t *handleName, uint8_t handleLen) {

	int pollReturn;
	
	uint8_t stdinBuf[MAXBUF];
	uint8_t sendBuf[MAXBUF];
	uint8_t tempBuf[MAXBUF];
	uint32_t stdinLen;
	uint32_t pckDataLen = 0; 
	uint8_t flag = 0;

	initialPacket(mainServerSocket, handleName);
	setupPollSet();
	addToPollSet(STDIN_FILENO);
	addToPollSet(mainServerSocket);
	
	// sends handle name to the server and waits for a response back
	
	
	// sendToServer(mainServerSocket);

	printf("Enter Data: ");
	fflush(stdout);
	while(1) {
		pckDataLen = stdinLen = 0;
		
		pollReturn = pollCall(-1);
		// fflush(stdout);
		if(pollReturn == -1) {
			perror("Timeout Error while Polling\n");
			exit(-1);
		}

		if(pollReturn == mainServerSocket) {
			// wait for client to connect
			processMsgFromServer(mainServerSocket, tempBuf);
			printf("\nEnter data: ");
			fflush(stdout);
			
		}
		else {
			pckDataLen += (2 + handleLen);
			stdinLen = readFromStdin(stdinBuf);
			pckDataLen += processStdin(sendBuf + PDU_HEADER_LEN + pckDataLen, stdinBuf, stdinLen, &flag);

			addSrcHandle(sendBuf, handleLen, handleName, flag);
			// buildPduPacket(sendBuf, pckDataLen, flag);
			sendPDU(mainServerSocket, sendBuf, pckDataLen);
		}
		
	}

}

uint8_t addSrcHandle(uint8_t *sendBuf, uint8_t handleLen, uint8_t *handleName, uint8_t flag) {
	memcpy(sendBuf + 2, &flag, 1);
	memcpy(sendBuf + 3, &handleLen, 1);
	memcpy(sendBuf + 4, handleName, handleLen);
	return handleLen + 2;
}

uint8_t getHandleName(uint8_t *inputData, uint8_t *destHandle) {

	uint8_t *tempHandle;
	tempHandle = (uint8_t*) strtok((char*) inputData, " ");

	memcpy(destHandle, tempHandle, strlen((char*) tempHandle));
	return strlen((char*) destHandle);
}

uint8_t getDestHandles(uint8_t *handleBuf, uint8_t *stdinBuf, uint8_t numHandles) {
	uint8_t destHandleLen;
	uint8_t totLen = 0;
	uint8_t destHandle[101];

	memcpy(handleBuf, &numHandles, 1);
	totLen+=1;
	handleBuf+=1;
	for(uint8_t i = 0; i < numHandles; i++) {
		destHandleLen = getHandleName(stdinBuf, destHandle);	// grabs the Handle name
		
		memcpy(handleBuf, &destHandleLen, 1);					// add handle length
		memcpy(handleBuf+1, destHandle, destHandleLen);			// add handle name
		handleBuf += (destHandleLen + 1);						// increments output data pointer with len + handle name
		totLen += (destHandleLen + 1);							// increments total length	
		stdinBuf += (destHandleLen + 1);						// increments stdin buffer with handle len + space
	}
	return totLen;
}



uint32_t processStdin(uint8_t *pckDataBuf, uint8_t *stdinBuf, uint32_t stdinLen, uint8_t *flag) {

	
	uint8_t destHandleLen = 0;
	uint32_t pckDataLen = 0;
	uint8_t numDestHandles = 1;
	uint8_t destHandles[918];		// max of 9 destination handles (100 bytes) + 9 length bytes

	// ERROR CHECK A PERCENT SIGN
	// foregoes the % sign in the STDIN buffer
	stdinBuf += 1;
	stdinLen -= 1;

	switch(tolower(stdinBuf[0])) {
		// foregoes the command and space in STDIN buffer

		case 'c':
			// find the number of dest handles
			
		case 'm':
			stdinBuf += 2;
			stdinLen -= 2;

			*flag = 5;
			destHandleLen = getDestHandles(destHandles, stdinBuf, numDestHandles);
			memcpy(pckDataBuf, destHandles, destHandleLen);
			pckDataLen += destHandleLen;			// keep track of total packet length
			
			stdinLen -= destHandleLen -1;				// remaining length is data length
			pckDataBuf += destHandleLen;
			stdinBuf += destHandleLen-1;
			memcpy(pckDataBuf, stdinBuf, stdinLen);
			
			pckDataLen += stdinLen;					// keep track of total packet length
			break;
		case 'b':
			break;
		case 'l':
			break;
		case 'e':
			break;
		
	}
	return pckDataLen;
	// printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
}


void fromServer(int clientSocket, uint8_t msgLen, uint8_t *dataBuf) {
	uint8_t *tempBuf = dataBuf;
	uint8_t flag = dataBuf[0];
	uint8_t srcHandleLen;


	uint8_t srcHandle[101];
	uint8_t destHandleLen;
	uint8_t destHandle[101];
	uint8_t numDestHandle = 1;
	uint32_t destSocket = 4;

	uint8_t *message;
	

	
	switch(flag) {

		case 7:
			destHandleLen = dataBuf[1];
			memcpy(destHandle, &dataBuf[2], destHandleLen);
			destHandle[destHandleLen] = '\0';
			printf("Client with handle %s does not exist\n", destHandle);
			break;

		case 5:
			srcHandleLen = dataBuf[1];
			memcpy(srcHandle, &dataBuf[2], srcHandleLen);
			srcHandle[srcHandleLen] = '\0';

			tempBuf += srcHandleLen + 2;
			numDestHandle = tempBuf[0];
			
			tempBuf += 1;
			while(numDestHandle != 0) {
				memcpy(&destHandleLen, tempBuf, 1);
				tempBuf += 1 + destHandleLen;
				numDestHandle -= 1;
			}

			printf("%s: %s\n", srcHandle, tempBuf);
			break;

	}
		
}



uint8_t processMsgFromServer(int mainServerSocket, uint8_t *retBuffer) {
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	// get message from the main server
	if ((messageLen = recvPDU(mainServerSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	
	if (messageLen > 0)
	{
		fromServer(mainServerSocket, messageLen, dataBuffer);
		// memcpy(retBuffer, dataBuffer, messageLen);
		// printf("From Server: %s of Length %d\n", dataBuffer, messageLen);
		// printf("\n\nEnter Data: ");
		return messageLen;
	
	}
	else
	{
		printf("Connection closed by server\n");
		removeFromPollSet(mainServerSocket);
		removeFromPollSet(STDIN_FILENO);
		close(mainServerSocket);
		exit(-1);
	}
}

void sendToServer(int socketNum, uint8_t *sendBuf, uint8_t sendLen) {
	// uint8_t sendBuf[MAXBUF];   	//data buffer
	// int sendLen = 0;        	//amount of data to send
	int sent = 0;            	//actual amount of data sent/* get the data and send it   */
	
	// sendLen = readFromStdin(sendBuf);
	// printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
	//sent =  safeSend(socketNum, sendBuf, sendLen, 0);
	sent =  sendPDU(socketNum, sendBuf, sendLen);
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	// printf("Amount of data sent is: %d\n", sent);
}

int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
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


