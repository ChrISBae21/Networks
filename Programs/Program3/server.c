/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "pollLib.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "swindow.h"

typedef struct bookKeep {
	uint32_t serverSeqNum;
} BookKeep;

BookKeep serverBook = {0};

typedef enum {
	START,
	FILENAME,
	USE,
	TEARDOWN,
	DONE
} STATE;




#define MAXBUF 1400
#define TIMEOUT -1
#define ONE_SEC 1000
#define ZERO_SEC 0

void processClient(int socketNum, float err);
int checkArgs(int argc, char *argv[]);
void childFSM(pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client);
void mainserver(int mainServerSocket, float err);
STATE filename(int *childSocket, pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client, FILE **fd);
void handleZombies(int sig);
void processSREJ(pduPacket *pduBuffer, int *pduLen, int childSocket, struct sockaddr_in6 *client);
void processRR(pduPacket *pduBuffer);

int main (int argc, char *argv[]) { 
	int socketNum = 0;				
	int portNumber = 0;
	float err = 0;

	portNumber = checkArgs(argc, argv);
	err = atof(argv[1]);
	sendErr_init(err, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	socketNum = udpServerSetup(portNumber);
	mainserver(socketNum, err);	

	close(socketNum);
		
	return 0;
}


void mainserver(int mainServerSocket, float err) {
	pduPacket pduBuffer;
	int pduLen, pid;
	struct sockaddr_in6 client;
	int clientAddrLen = sizeof(client);	

	handleZombies(SIGCHLD);
	// signal(SIGCHLD, handleZombies);
	addToPollSet(mainServerSocket);

	while(1) {
		pollCall(-1);
		pduLen = safeRecvfrom(mainServerSocket, &pduBuffer, MAX_PDU, 0, (struct sockaddr *) &client, &clientAddrLen);
		if((pid = fork()) < 0) {
			perror("fork");
			exit(-1);
		}
		if(pid == 0) {
			close(mainServerSocket);
			removeFromPollSet(mainServerSocket);
			sendErr_init(err, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
			childFSM(&pduBuffer, &pduLen, &client);
			exit(0);
		}
	}
}

STATE filename(int *childSocket, pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client, FILE **fd) {
	char fileName[101];
	uint8_t fnameAckPayload = 0;
	uint16_t windowSize;
	uint32_t payloadSize;

	if(in_cksum((unsigned short*)pduBuffer, *pduLen))  {
		return DONE;
	}
	/* grab the file name */
	getFromFileName(pduBuffer, *pduLen, fileName);
	windowSize = getWindowSizeFromPDU(pduBuffer);
	payloadSize = getPayloadSizeFromPDU(pduBuffer);

	/* create a new socket */
	*childSocket = safeSocket();
	
	/* check if the file exists */
	if((*fd = fopen(fileName, "rb")) == NULL) {

		*pduLen = createPDU(pduBuffer, serverBook.serverSeqNum, FLAG_FILENAME_ACK, &fnameAckPayload, 1);
		safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
		return DONE;
	}



	fnameAckPayload = 1;
	*pduLen = createPDU(pduBuffer, serverBook.serverSeqNum++, FLAG_FILENAME_ACK, &fnameAckPayload, 1);
	
	/* DEBUG */
	// *pduLen = createPDU(pduBuffer, 0, FLAG_FILENAME_ACK, &fnameAckPayload, 1);
	// safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));

	// *pduLen = createPDU(pduBuffer, 1, FLAG_DATA, &fnameAckPayload, 1);
	// safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));

	// *pduLen = createPDU(pduBuffer, 2, FLAG_DATA, &fnameAckPayload, 1);
	
	/* sends a good file name packet */
	safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
	initWindow(windowSize, payloadSize);
	return USE;
}

STATE use(pduPacket *pduBuffer, int *pduLen, FILE **fd, int childSocket, struct sockaddr_in6 *client) {
	uint8_t EOF_READY = 0;
	uint8_t retryCount = 0;
	size_t bytesRead = 0;
	uint8_t fileData[MAX_PAYLOAD];

	while(!EOF_READY) {
		retryCount = 0;
		while(getWindowStatus() && !EOF_READY) {
			if( (bytesRead = fread(fileData, sizeof(uint8_t), getPayloadSize(), *fd)) < getPayloadSize() ) {
				EOF_READY = 1;
			}
			// CREATE PDU
			*pduLen = createPDU(pduBuffer, serverBook.serverSeqNum++, FLAG_DATA, fileData, bytesRead);
			// STORE PDU IN WINDOW
			storePDUWindow(pduBuffer, *pduLen, ntohl(pduBuffer->nSeqNo));
			// SEND DATA
			safeSendto(childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));

			while(pollCall(ZERO_SEC) != TIMEOUT) {
				switch(pduBuffer->flag) {
					case FLAG_RR:
					processRR(pduBuffer);
					break;
					case FLAG_SREJ:
					processSREJ(pduBuffer, pduLen, childSocket, client);
					break;
				}
			}

		}
		while(!getWindowStatus() && !EOF_READY) {
			if(pollCall(ONE_SEC) != TIMEOUT) {
				switch(pduBuffer->flag) {
				case FLAG_RR:
				processRR(pduBuffer);
				break;
				case FLAG_SREJ:
				processSREJ(pduBuffer, pduLen, childSocket, client);
				break;
				}
			}
			else {
				retryCount++;
				*pduLen = getLowest(pduBuffer);
				// SEND LOWEST PACKET
			}
		}
	}

	return TEARDOWN;
}

void processRR(pduPacket *pduBuffer) {
	uint32_t nRR;
	memcpy(&nRR, pduBuffer->payload, 4);
	slideWindow(ntohl(nRR));
}

void processSREJ(pduPacket *pduBuffer, int *pduLen, int childSocket, struct sockaddr_in6 *client) {
	uint32_t nSREJ;
	memcpy(&nSREJ, pduBuffer, 4);
	*pduLen = getPDUWindow(pduBuffer, ntohl(nSREJ));

	//maybe bug here where payload address is the same
	*pduLen = createPDU(pduBuffer, serverBook.serverSeqNum++, FLAG_SREJ_DATA, pduBuffer->payload, *pduLen);
	safeSendto(childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
}




void childFSM(pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client) {
	STATE state = START;
	int childSocket;
	FILE *fd;

		while(state != DONE) {
			switch(state) {
				case START:
				state = FILENAME;
				break;
				case FILENAME:
				state = filename(&childSocket, pduBuffer, pduLen, client, &fd);
				break;
				case USE:
				// state = DONE;
				// state = inorder();
				break;
				// state = buffer();
				break;
				case TEARDOWN:
				state = DONE;
				// state = flush();
				break;
				case DONE:
				break;
			}
	}


}

int checkArgs(int argc, char *argv[]) {
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 3 || argc < 2) {
		fprintf(stderr, "Usage %s [error rate] [optional port number]\n", argv[0]);
		exit(-1);
	}
	if (argc == 3) {
		portNumber = atoi(argv[2]);
	}
	
	return portNumber;
}


void handleZombies(int sig) {
	int stat = 0;
	while(waitpid(-1, &stat, WNOHANG) > 0);
}
	