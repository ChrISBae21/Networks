// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

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

#include "pollLib.h"
#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"



#define MAXBUF 1400
#define TIMEOUT -1
#define ONE_SEC 1000

typedef enum {
	FILENAME, 
	FILENAME_ACK,
	INORDER, 
	BUFFER, 
	FLUSH, 
	DONE
} STATE;


void talkToServer(int socketNum, struct sockaddr_in6 * server);
int readFromStdin(char * buffer);
int checkArgs(int argc, char * argv[]);
void downloadFSM(char* argv[], int portNumber);
void cleanSocket(int socket);
STATE filenameAck(char* argv[], pduPacket *pduBuffer, uint32_t *expected, struct sockaddr_in6 *server, int *socketNum, uint8_t *fnameRetry, FILE **fd, uint32_t *rcopySeqNum);
STATE filename(char* argv[], pduPacket *pduBuffer, struct sockaddr_in6 *server, int portNumber, uint16_t bufferSize, uint32_t windowSize, int *socketNum, uint8_t *fnameRetry, uint32_t *rcopySeqNum);


// <from-filename> <to-filename> <window-size> <buffer-size> <error-rate> <IP> <port #>

int main (int argc, char *argv[]) {
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);
	
	sendErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	setupPollSet();
	downloadFSM(argv, portNumber);
	// socketNum = setupUdpClientToServer(&server, argv[2], portNumber);
	
	// talkToServer(socketNum, &server);
	
	// close(socketNum);

	return 0;
}


STATE filename(char* argv[], pduPacket *pduBuffer, struct sockaddr_in6 *server, int portNumber, uint16_t bufferSize, uint32_t windowSize, int *socketNum, uint8_t *fnameRetry, uint32_t *rcopySeqNum) {
	int pduLen, payloadLen;

	if(*fnameRetry > 9) return DONE;

	/* open the socket */
	*socketNum = setupUdpClientToServer(server, argv[6], portNumber);
	addToPollSet(*socketNum);

	/* create the filename payload */
	memcpy(pduBuffer->payload, &bufferSize, 2);
	memcpy(pduBuffer->payload + 2, &windowSize, 4);
	memcpy(pduBuffer->payload + 6, argv[1], strlen(argv[1]));
	payloadLen = 6 + strlen(argv[1]);

	/* create the PDU */
	pduLen = createPDU((uint8_t*)pduBuffer, *rcopySeqNum, FLAG_FILENAME, pduBuffer->payload, payloadLen);
	/* send the PDU */
	safeSendto(*socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) server, sizeof(struct sockaddr_in6));

	/* timed out */
	if(pollCall(ONE_SEC) == TIMEOUT) {
		/* debug */
		printf("Timeout\n");
		cleanSocket(*socketNum);
		(*fnameRetry)++;
		return FILENAME;
	}

	/* debug */
	printf("Got a Packet\n");
	/* Received something */
	return FILENAME_ACK;

}

STATE filenameAck(char* argv[], pduPacket *pduBuffer, uint32_t *expected, struct sockaddr_in6 *server, int *socketNum, uint8_t *fnameRetry, FILE **fd, uint32_t *rcopySeqNum) {
	int pduLen;
	uint32_t serverSeqNum;
	int serverAddrLen = sizeof(struct sockaddr_in6);

	pduLen = safeRecvfrom(*socketNum, pduBuffer, MAX_PDU, 0, (struct sockaddr *) server, &serverAddrLen);
	/* corrupted packet */
	if(in_cksum((unsigned short*)pduBuffer, pduLen))  {
		/* debug */
		printf("Corrupted Packet\n");
		cleanSocket(*socketNum);
		(*fnameRetry)++;
		return FILENAME;
	}

	// /* try to open the file to write to */
	// *fd = fopen(argv[2], "wb");
	if((*fd = fopen(argv[2], "wb")) == NULL) {
		printf("Error opening file to write to\n");
		return DONE;
	}
	
	serverSeqNum = getHSeqNum((uint8_t *)pduBuffer);

	/* got a packet, but it was a packet greater than seq 1 */
	if(serverSeqNum > 1) {
		/* debug */
		printf("Seq Greater than 1\n");
		return BUFFER;
	}
	/* got first data packet */
	if(serverSeqNum == 1) {
		/* debug */
		printf("Seq Equals 1\n");
		(*expected)++;
		return INORDER;
	}
	/* got the filename ack */
	if(pduBuffer->payload[0] && (pduBuffer->flag == FLAG_FILENAME_ACK)) {
		/* debug */
		printf("Got Good Filename Ack\n");
		return INORDER;
	}
	/* debug */
	printf("Got Bad Filename Ack\n");
	return DONE;
}

void cleanSocket(int socket) {
	close(socket);
	removeFromPollSet(socket);
}

void downloadFSM(char* argv[], int portNumber) {
	struct sockaddr_in6 server 	= {0};		// Supports 4 and 6 but requires IPv6 struct
	uint16_t bufferSize 	= atoi(argv[4]);
	uint32_t windowSize 	= atoi(argv[3]);
	int socketNum;
	FILE *fd;
	uint8_t fnameRetry		= 0;
	// uint32_t rcopySeqNum, serverSeqNum;
	uint32_t rcopySeqNum;
	uint32_t expected = 1;
	pduPacket pduBuffer;
	STATE state 			= FILENAME;

	while(state != DONE) {
		switch(state) {
			case FILENAME:
			/* debug */
			printf("FILENAME\n");
			state = filename(argv, &pduBuffer, &server, portNumber, bufferSize, windowSize, &socketNum, &fnameRetry, &rcopySeqNum);
			break;
			case FILENAME_ACK:
			/* debug */
			printf("FILENAME ACK\n");
			state = filenameAck(argv, &pduBuffer, &expected, &server, &socketNum, &fnameRetry, &fd, &rcopySeqNum);
			break;
			case INORDER:
			state = DONE;
			// state = inorder();
			break;
			case BUFFER:
			state = DONE;
			// state = buffer();
			break;
			case FLUSH:
			state = DONE;
			// state = flush();
			break;
			case DONE:
			break;
		}
	}
	cleanSocket(socketNum);

}


void talkToServer(int socketNum, struct sockaddr_in6 * server) {
	int serverAddrLen = sizeof(struct sockaddr_in6);
	int dataLen = 0; 
	char buffer[MAXBUF+1];
	char payload[MAXBUF+1];
	
	buffer[0] = '\0';
	while (buffer[0] != '.') {
		// dataLen = readFromStdin(payload);
		dataLen = createPDU((uint8_t *)buffer, 1, 1, (uint8_t *)payload, dataLen);
		printf("Sending: \n---------------------------------------\n");
		printPDU((uint8_t *)buffer, dataLen);
		printf("---------------------------------------\n");
		printf("\n");
		// printf("Sending: %s with len: %d\n", buffer,dataLen);
	
		safeSendto(socketNum, buffer, dataLen, 0, (struct sockaddr *) server, serverAddrLen);
	      
	}
}


int checkArgs(int argc, char * argv[]) {
    int portNumber = 0;
	
    /* check command line arguments  */
	if (argc != 8) {
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
	portNumber = atoi(argv[7]);	
	return portNumber;
}





