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
uint8_t addSrcHandle(uint8_t *sendBuf, uint8_t handleLen, uint8_t *handleName);

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

	
	addSrcHandle(dataBuffer, strlen((char*) handleName), handleName);

	// builds the PDU Chat header and the PDU packet
	buildPduPacket(dataBuffer, strlen((char*)handleName) + 1, 1);

	// +1 len is for the handle length byte
	sendToServer(mainServerSocket, dataBuffer, strlen((char*) handleName));

	msgLen = processMsgFromServer(mainServerSocket, &flag);

	fprintf(stderr, "msgLen: %d\n", msgLen);

	if(flag == 2) {
		printf("Handle: %s has been verified by the server\n", handleName);
	}
	else if(flag == 3) {
		printf("Handle: %s has been taken\n", handleName);
		exit(-1);
	}
	// NEED TO RECIEVE THE PDU FROM THE SERVER AND VERIFY THE FLAG !!!!!!!!!!!!!!!!!!!!!!!!!!!
}

void clientControl(int mainServerSocket, uint8_t *handleName, uint8_t handleLen) {

	int pollReturn;
	uint8_t stdinBuf[MAXBUF];
	uint8_t sendBuf[MAXBUF];
	uint32_t stdinLen, pckDataLen;
	uint8_t flag = 0;

	setupPollSet();
	addToPollSet(STDIN_FILENO);
	addToPollSet(mainServerSocket);
	
	// sends handle name to the server and waits for a response back
	initialPacket(mainServerSocket, handleName);
	
	// sendToServer(mainServerSocket);

	printf("Enter Data: ");
	fflush(stdout);
	while(1) {
		
		pollReturn = pollCall(-1);
		// fflush(stdout);
		if(pollReturn == -1) {
			perror("Timeout Error while Polling\n");
			exit(-1);
		}

		if(pollReturn == mainServerSocket) {
			// wait for client to connect
			processMsgFromServer(mainServerSocket, NULL);
			printf("\nEnter data: ");
			fflush(stdout);
			
		}
		else {
			pckDataLen = addSrcHandle(sendBuf, handleLen, handleName);
			stdinLen = readFromStdin(stdinBuf);
			pckDataLen += processStdin(sendBuf + pckDataLen, stdinBuf, stdinLen, &flag);
			buildPduPacket(sendBuf, pckDataLen, flag);
			sendPDU(mainServerSocket, sendBuf, pckDataLen);
		}
		
	}

}

uint8_t addSrcHandle(uint8_t *sendBuf, uint8_t handleLen, uint8_t *handleName) {
	memcpy(sendBuf + 3, &handleLen, 1);
	memcpy(sendBuf + 1, handleName, handleLen);
	return handleLen + 1;
}

uint8_t getHandleName(uint8_t *inputData, uint8_t *destHandle) {
	destHandle = (uint8_t*) strtok((char*) inputData, " ");
	return strlen((char*) destHandle);
}

uint8_t getDestHandles(uint8_t *handleBuf, uint8_t *stdinBuf, uint8_t numHandles) {
	uint8_t destHandleLen;
	uint8_t totLen = 0;
	uint8_t destHandle[101];

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

	uint8_t destHandles[918];		// max of 9 destination handles (100 bytes) + 9 length bytes

	// ERROR CHECK A PERCENT SIGN
	// foregoes the % sign in the STDIN buffer
	stdinBuf += 1;
	stdinLen -= 1;

	switch(tolower(stdinBuf[0])) {
		// foregoes the command and space in STDIN buffer
		case 'm':
			stdinBuf += 2;
			stdinLen -= 2;
			*flag = 5;
			destHandleLen = getDestHandles(destHandles, stdinBuf, 1);
			pckDataLen += destHandleLen;			// keep track of total packet length
			stdinLen -= destHandleLen;				// remaining length is data length
			pckDataBuf += destHandleLen;
			stdinBuf += destHandleLen;
			memcpy(pckDataBuf, stdinBuf, stdinLen);

			pckDataLen += stdinLen;					// keep track of total packet length
			break;
		case 'b':
			break;

		case 'c':
			break;
		case 'l':
			break;
		case 'e':
			break;
		
	}
	return pckDataLen;
	// printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
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
		//memcpy(retBuffer, dataBuffer, messageLen);
		printf("From Server: %s of Length %d\n", dataBuffer, messageLen);
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

	printf("Amount of data sent is: %d\n", sent);
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
