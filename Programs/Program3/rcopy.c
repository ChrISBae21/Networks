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
#include "swindow.h"


#define RR_PAYLOAD_LEN 4
#define SREJ_PAYLOAD_LEN 4
#define MAXBUF 1400
#define TIMEOUT -1
#define ONE_SEC 1000
#define TEN_SEC 10000


typedef struct bookKeep {
	uint32_t highest;
	uint32_t expected;
	uint32_t rcopySeqNum;
	uint8_t eof_flag;
	uint8_t error;
} BookKeep;

typedef enum {
	FILENAME, 
	FILENAME_ACK,
	FILENAME_OK,
	INORDER, 
	BUFFER, 
	FLUSH, 
	TEARDOWN,
	DONE
} STATE;

BookKeep Book = {1, 1, 0, 0, 0};

void talkToServer(int socketNum, struct sockaddr_in6 * server);
int readFromStdin(char * buffer);
int checkArgs(int argc, char * argv[]);
void downloadFSM(char* argv[], int portNumber);
void cleanSocket(int socket);
STATE filenameAck(char* argv[], pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *server, int *socketNum, uint8_t *fnameRetry, FILE **fd);
STATE filename(char* argv[], pduPacket *pduBuffer, struct sockaddr_in6 *server, int portNumber, uint16_t bufferSize, uint32_t windowSize, int *socketNum, uint8_t *fnameRetry);
STATE filenameOk(pduPacket *pduBuffer, uint32_t windowSize, uint16_t packetSize, int *pduLen, int* socketNum, struct sockaddr_in6 *server, FILE **fd);
STATE inorder(pduPacket *pduBuffer, int *pduLen, FILE **fd, int *socketNum, struct sockaddr_in6 *server);
STATE teardown(pduPacket *pduBuffer, int *pduLen, FILE **fd, int *socketNum, struct sockaddr_in6 *server);
STATE buffer(pduPacket *pduBuffer, int *pduLen, FILE **fd, int socketNum, struct sockaddr_in6 *server);
STATE flush(pduPacket *pduBuffer, int *pduLen, FILE **fd, int socketNum, struct sockaddr_in6 *server);
int RR_SREJ(pduPacket *pduBuffer, int flag, uint32_t nrr_srej);

int main (int argc, char *argv[]) {
	int portNumber = 0;
	portNumber = checkArgs(argc, argv);
	sendErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	setupPollSet();
	downloadFSM(argv, portNumber);
	freePollSet();	
	return 0;
}


STATE filename(char* argv[], pduPacket *pduBuffer, struct sockaddr_in6 *server, int portNumber, uint16_t bufferSize, uint32_t windowSize, int *socketNum, uint8_t *fnameRetry) {
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
	pduLen = createPDU(pduBuffer, Book.rcopySeqNum, FLAG_FILENAME, pduBuffer->payload, payloadLen);
	/* send the PDU */
	safeSendto(*socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) server, sizeof(*server));

	/* timed out */
	if(pollCall(ONE_SEC) == TIMEOUT) {
		cleanSocket(*socketNum);
		(*fnameRetry)++;
		return FILENAME;
	}
	/* Received something */
	return FILENAME_ACK;

}

STATE filenameAck(char* argv[], pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *server, int *socketNum, uint8_t *fnameRetry, FILE **fd) {
	int serverAddrLen = sizeof(struct sockaddr_in6);

	*pduLen = safeRecvfrom(*socketNum, pduBuffer, MAX_PDU, 0, (struct sockaddr *) server, &serverAddrLen);
	
	/* corrupted packet */
	if(in_cksum((unsigned short*)pduBuffer, *pduLen))  {
		cleanSocket(*socketNum);
		(*fnameRetry)++;
		return FILENAME;
	}

	/* bad file name*/
	if((pduBuffer->flag == FLAG_FILENAME_ACK) && (!pduBuffer->payload[0]) ) {
		printf("File on the Server did not exist\n");
		return DONE;
	}
	*fd = fopen(argv[2], "w");
	return FILENAME_OK;
}

STATE filenameOk(pduPacket *pduBuffer, uint32_t windowSize, uint16_t packetSize, int *pduLen, int* socketNum, struct sockaddr_in6 *server, FILE **fd) {
	initBuffer(windowSize, packetSize);

	Book.rcopySeqNum++;
	switch(pduBuffer->flag) {
		/* packet is the filename ack */
		case FLAG_FILENAME_ACK:
			/* good file name */
			if(pduBuffer->payload[0]) return INORDER;
		break;

		case FLAG_EOF:
		Book.eof_flag = 1;
		/* packet is data */
		case FLAG_DATA:
		/* not the first data packet */
			if(ntohl(pduBuffer->nSeqNo) > 1)  {
				*pduLen = RR_SREJ(pduBuffer, FLAG_SREJ, Book.expected);
				addToBuffer(pduBuffer, *pduLen, ntohl(pduBuffer->nSeqNo));
				Book.highest = ntohl(pduBuffer->nSeqNo);	
				safeSendto(*socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
				return BUFFER;
			}
			
			/* first data packet */
			fwrite(pduBuffer->payload, sizeof(uint8_t), (size_t)(*pduLen - PDU_HEADER_LEN), *fd);
			Book.highest = Book.expected;
			Book.expected++;
			*pduLen = RR_SREJ(pduBuffer, FLAG_RR, Book.expected);
			safeSendto(*socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));

			return INORDER;
		break;
	}
	return FILENAME_ACK;
}

STATE inorder(pduPacket *pduBuffer, int *pduLen, FILE **fd, int *socketNum, struct sockaddr_in6 *server) {
	STATE returnValue = INORDER;
	int serverAddrLen = sizeof(*server);
	uint32_t hSeqNo;

	if(Book.eof_flag == 1) {
		return TEARDOWN;
	}
	if(pollCall(TEN_SEC) == TIMEOUT) {
		Book.error = 1;
		return DONE;
	}
	*pduLen = safeRecvfrom(*socketNum, pduBuffer, MAX_PDU, 0, (struct sockaddr *) server, &serverAddrLen);

	if(in_cksum((unsigned short*)pduBuffer, *pduLen))  {
		return INORDER;
	}
	hSeqNo = ntohl(pduBuffer->nSeqNo);
	
	if(pduBuffer->flag == FLAG_EOF) {
		Book.eof_flag = 1;
	}
	if(hSeqNo == Book.expected) {
		fwrite(pduBuffer->payload, sizeof(uint8_t), (size_t)(*pduLen - PDU_HEADER_LEN), *fd);
		Book.highest = Book.expected;
		Book.expected++;
		*pduLen = RR_SREJ(pduBuffer, FLAG_RR, Book.expected);	// RR
		safeSendto(*socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		returnValue = INORDER;
	}

	else if(hSeqNo > Book.expected) {
		addToBuffer(pduBuffer, *pduLen, hSeqNo);
		Book.highest = hSeqNo;	
		*pduLen = RR_SREJ(pduBuffer, FLAG_SREJ, Book.expected);	// SREJ
		safeSendto(*socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		returnValue = BUFFER;
	}
	else if(hSeqNo < Book.expected) {
		*pduLen = RR_SREJ(pduBuffer, FLAG_SREJ, Book.expected);
		safeSendto(*socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		*pduLen = RR_SREJ(pduBuffer, FLAG_RR, Book.expected);
		safeSendto(*socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		returnValue = INORDER;
	}
	return returnValue;
}

STATE buffer(pduPacket *pduBuffer, int *pduLen, FILE **fd, int socketNum, struct sockaddr_in6 *server) {
	STATE returnValue = BUFFER;
	int serverAddrLen = sizeof(*server);
	uint32_t hSeqNo;
	
	if(pollCall(TEN_SEC) == TIMEOUT) {
		Book.error = 1;
		return DONE;
	}	
	*pduLen = safeRecvfrom(socketNum, pduBuffer, MAX_PDU, 0, (struct sockaddr *) server, &serverAddrLen);

	if(in_cksum((unsigned short*)pduBuffer, *pduLen))  {
		return BUFFER;
	}

	hSeqNo = ntohl(pduBuffer->nSeqNo);
	if(pduBuffer->flag == FLAG_EOF) {
		Book.eof_flag = 1;
	}
	if(hSeqNo == Book.expected) {
		fwrite(pduBuffer->payload, sizeof(uint8_t), (size_t)(*pduLen - PDU_HEADER_LEN), *fd);
		Book.expected++;
		returnValue = FLUSH;
	}

	else if(hSeqNo > Book.expected) {
		addToBuffer(pduBuffer, *pduLen, hSeqNo);
		Book.highest = hSeqNo;	
		returnValue = BUFFER;
	}

	else if(hSeqNo < Book.expected) {
		*pduLen = RR_SREJ(pduBuffer, FLAG_SREJ, Book.expected);
		safeSendto(socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		*pduLen = RR_SREJ(pduBuffer, FLAG_RR, Book.expected);
		safeSendto(socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		returnValue = BUFFER;
	}
	return returnValue;
}

STATE flush(pduPacket *pduBuffer, int *pduLen, FILE **fd, int socketNum, struct sockaddr_in6 *server) {
	STATE returnValue = BUFFER;

	while(checkValidPDU(Book.expected)) {
		*pduLen = getPDUFromBuffer(pduBuffer, Book.expected);
		removeFromBuffer(Book.expected);
		fwrite(pduBuffer->payload, sizeof(uint8_t), (size_t)(*pduLen - PDU_HEADER_LEN), *fd);
		Book.expected++;
	}

	if(Book.expected < Book.highest) {
		*pduLen = RR_SREJ(pduBuffer, FLAG_SREJ, Book.expected);
		safeSendto(socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		*pduLen = RR_SREJ(pduBuffer, FLAG_RR, Book.expected);
		safeSendto(socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		returnValue = BUFFER;
	}
	if((Book.expected-1) == Book.highest) {
		*pduLen = RR_SREJ(pduBuffer, FLAG_RR, Book.expected);
		safeSendto(socketNum, pduBuffer, *pduLen, 0, (struct sockaddr *) server, sizeof(*server));
		returnValue = INORDER;
	}
	return returnValue;
}

STATE teardown(pduPacket *pduBuffer, int *pduLen, FILE **fd, int *socketNum, struct sockaddr_in6 *server) {
	*pduLen = createPDU(pduBuffer, Book.rcopySeqNum, FLAG_EOF_ACK, pduBuffer->payload, 0);
	safeSendto(*socketNum, pduBuffer, PDU_HEADER_LEN, 0, (struct sockaddr *) server, sizeof(*server));
	return DONE;
}

void downloadFSM(char* argv[], int portNumber) {
	struct sockaddr_in6 server 	= {0};		// Supports 4 and 6 but requires IPv6 struct
	uint16_t packetSize 	= atoi(argv[4]);
	uint32_t windowSize 	= atoi(argv[3]);
	int socketNum;
	FILE *fd;

	uint8_t fnameRetry	= 0;
	int pduLen;
	pduPacket pduBuffer;
	STATE state = FILENAME;

	while(state != DONE) {
		switch(state) {
			case FILENAME:
			state = filename(argv, &pduBuffer, &server, portNumber, packetSize, windowSize, &socketNum, &fnameRetry);
			break;
			case FILENAME_ACK:
			state = filenameAck(argv, &pduBuffer, &pduLen, &server, &socketNum, &fnameRetry, &fd);
			break;
			case FILENAME_OK:
			state = filenameOk(&pduBuffer, windowSize, packetSize, &pduLen, &socketNum, &server, &fd);
			case INORDER:
			state = inorder(&pduBuffer, &pduLen, &fd, &socketNum, &server);
			break;
			case BUFFER:
			state = buffer(&pduBuffer, &pduLen, &fd, socketNum, &server);
			break;
			case FLUSH:
			state = flush(&pduBuffer, &pduLen, &fd, socketNum, &server);
			break;
			case TEARDOWN:
			state = teardown(&pduBuffer, &pduLen, &fd, &socketNum, &server);
			break;
			case DONE:
			break;
		}
	}
	
	cleanSocket(socketNum);
	if(Book.eof_flag || Book.error) {
		teardownBuffer();
		fclose(fd);
	}
}


int checkArgs(int argc, char * argv[]) {
    int portNumber = 0;
	
    /* check command line arguments  */
	if (argc != 8) {
		printf("usage: %s [from file] [to file] [window size] [buffer size] [error rate] [server] [server port number] \n", argv[0]);
		exit(1);
	}

	if(atoi(argv[4]) > 1400) {
		printf("Buffer Size must be less than or equal to 1400 bytes\n");
		exit(1);
	}
	if(atof(argv[5]) > 1) {
		printf("Error Rate must be a a value between 0 and 1\n");
		exit(1);
	}
	portNumber = atoi(argv[7]);	
	return portNumber;
}

void cleanSocket(int socket) {
	close(socket);
	removeFromPollSet(socket);
}

int RR_SREJ(pduPacket *pduBuffer, int flag, uint32_t hrr_srej) {
	uint32_t nrr_srej = htonl(hrr_srej);
	return createPDU(pduBuffer, Book.rcopySeqNum++, flag, (uint8_t*) &nrr_srej, RR_PAYLOAD_LEN);
}
