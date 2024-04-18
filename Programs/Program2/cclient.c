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

#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "pollLib.h"


#define MAXBUF 1400
#define DEBUG_FLAG 1

void sendToServer(int socketNum, uint8_t *sendBuf, uint8_t sendLen);
int readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void clientControl(int mainServerSocket, char *handleName, uint8_t handleLen);
uint8_t processMsgFromServer(int mainServerSocket, uint8_t *retBuffer);
void processStdin(int mainServerSocket);
void initialPacket(int mainServerSocket, char *handleName);


int main(int argc, char * argv[]) {
	int socketNum = 0;         //socket descriptor
	checkArgs(argc, argv);
	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	clientControl(socketNum, argv[1], strlen(argv[1]));
	return 0;
}



void initialPacket(int mainServerSocket, char *handleName) {
	uint8_t dataBuffer[MAXBUF] = {};
	uint8_t msgLen;
	uint8_t flag = 0;

	
	// builds the PDU Chat header and the PDU packet
	buildEntirePDU(dataBuffer, handleName, strlen(handleName), 1);

	// +1 len is for the handle length byte
	sendToServer(mainServerSocket, dataBuffer, strlen(handleName) + 1);

	msgLen = processMsgFromServer(mainServerSocket, &flag);

	if(flag == 2) {
		printf("Handle: %s has been verified by the server\n", handleName);
	}
	else if(flag == 3) {
		printf("Handle: %s has been taken\n", handleName);
		exit(-1);
	}
	// NEED TO RECIEVE THE PDU FROM THE SERVER AND VERIFY THE FLAG !!!!!!!!!!!!!!!!!!!!!!!!!!!
}

void clientControl(int mainServerSocket, char *handleName, uint8_t handleLen) {

	int pollReturn;
	
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
			processMsgFromServer(mainServerSocket);
			printf("\nEnter data: ");
			fflush(stdout);
			
		}
		else {
			processStdin(mainServerSocket);
		}
		
	}

}

void processStdin(int mainServerSocket) {
	uint8_t sendBuf[MAXBUF];   	//data buffer
	int sendLen = 0;        	//amount of data to send
	
	sendLen = readFromStdin(sendBuf);
	// printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	sendToServer(mainServerSocket, sendBuf, sendLen);
	
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
		memcpy(retBuffer, dataBuffer, messageLen);
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
