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
	uint32_t eofSeqNo;
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
void childFSM(pduPacket *pduBuffer, uint16_t *pduLen, struct sockaddr_in6 *client);
void mainserver(int mainServerSocket, float err);
STATE filename(int *childSocket, pduPacket *pduBuffer, uint16_t *pduLen, struct sockaddr_in6 *client, FILE **fd);
void handleZombies(int sig);
void processSREJ(pduPacket *pduBuffer, uint16_t *pduLen, int childSocket, struct sockaddr_in6 *client);
uint32_t processRR(pduPacket *pduBuffer);

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
	int pid;
	struct sockaddr_in6 client;
	int clientAddrLen = sizeof(client);	
	uint16_t pduLen;

	signal(SIGCHLD, handleZombies);

	while(1) {
		pduLen = safeRecvfrom(mainServerSocket, &pduBuffer, MAX_PDU, 0, (struct sockaddr *) &client, &clientAddrLen);
		if(!in_cksum((unsigned short*)&pduBuffer, pduLen))  {
			if((pid = fork()) < 0) {
				perror("fork");
				exit(-1);
			}
			if(pid == 0) {
				close(mainServerSocket);
				sendErr_init(err, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
				// sendErr_init(err, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
				childFSM(&pduBuffer, &pduLen, &client);
				exit(0);
			}
		}
	}
}

STATE filename(int *childSocket, pduPacket *pduBuffer, uint16_t *pduLen, struct sockaddr_in6 *client, FILE **fd) {
	char fileName[101];
	uint8_t fnameAckPayload = 0;
	uint32_t windowSize;
	uint16_t payloadSize;

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
		close(*childSocket);
		return DONE;
	}

	fnameAckPayload = 1;
	*pduLen = createPDU(pduBuffer, serverBook.serverSeqNum++, FLAG_FILENAME_ACK, &fnameAckPayload, 1);
	
	/* sends a good file name packet */
	setupPollSet();
	addToPollSet(*childSocket);
	safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
	initWindow(windowSize, payloadSize);
	return USE;
}

STATE use(pduPacket *pduBuffer, uint16_t *pduLen, FILE **fd, int childSocket, struct sockaddr_in6 *client) {
	uint8_t EOF_READY = 0;
	uint8_t retryCount = 0;
	uint8_t flag = FLAG_DATA;
	size_t bytesRead = 0;
	uint8_t fileData[MAX_PAYLOAD];
	int clientAddrLen = sizeof(*client);	


	while(!EOF_READY) {
		/* window open */
		while(getWindowStatus() && !EOF_READY) {
			if( (bytesRead = fread(fileData, sizeof(uint8_t), getPayloadSize(), *fd)) < getPayloadSize() ) {
				EOF_READY = 1;
				flag = FLAG_EOF;
				serverBook.eofSeqNo = serverBook.serverSeqNum;
			}

			// CREATE PDU
			*pduLen = createPDU(pduBuffer, serverBook.serverSeqNum++, flag, fileData, bytesRead);
			// STORE PDU IN WINDOW
			storePDUWindow(pduBuffer, *pduLen, ntohl(pduBuffer->nSeqNo));
			// SEND DATA
			safeSendto(childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
			while((pollCall(ZERO_SEC) != TIMEOUT)  && !EOF_READY) {
				*pduLen = safeRecvfrom(childSocket, pduBuffer, MAX_PDU, 0, (struct sockaddr *) client, &clientAddrLen);
				retryCount = 0;
				if(!in_cksum((unsigned short*)pduBuffer, *pduLen))  {
					switch(pduBuffer->flag) {
						case FLAG_RR:
						processRR(pduBuffer);
						break;
						case FLAG_SREJ:
						processSREJ(pduBuffer, pduLen, childSocket, client);
						break;
						default:
						break;
					}
				}
			}
		}
		/* window closed */
		while(!getWindowStatus() && !EOF_READY) {
			if(retryCount > 9) return DONE;
			if(pollCall(ONE_SEC) != TIMEOUT) {
				*pduLen = safeRecvfrom(childSocket, pduBuffer, MAX_PDU, 0, (struct sockaddr *) client, &clientAddrLen);
				retryCount = 0;
				if(!in_cksum((unsigned short*)pduBuffer, *pduLen))  {
					switch(pduBuffer->flag) {
					case FLAG_RR:
					processRR(pduBuffer);
					break;
					case FLAG_SREJ:
					processSREJ(pduBuffer, pduLen, childSocket, client);
					break;
					default:
					break;
					}
				}
			}
			else {
				retryCount++;
				*pduLen = getLowest(pduBuffer);

				setFlag(pduBuffer, *pduLen, FLAG_TIMEOUT_DATA);
				safeSendto(childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
			}
		}
	}
	return TEARDOWN;
}

/* returns the seq no RR'ed */
uint32_t processRR(pduPacket *pduBuffer) {
	uint32_t nRR;
	memcpy(&nRR, pduBuffer->payload, 4);
	slideWindow(ntohl(nRR));
	return ntohl(nRR);
}


void processSREJ(pduPacket *pduBuffer, uint16_t *pduLen, int childSocket, struct sockaddr_in6 *client) {
	uint32_t nSREJ;
	memcpy(&nSREJ, pduBuffer->payload, 4);
	*pduLen = getPDUWindow(pduBuffer, ntohl(nSREJ));
	setFlag(pduBuffer, *pduLen, FLAG_SREJ_DATA);
	safeSendto(childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
}


STATE teardown(pduPacket *pduBuffer, uint16_t *pduLen, int childSocket, struct sockaddr_in6 *client, FILE **fd) {
	uint8_t retryCount = 0;
	uint8_t eof_ack_flag = 0;
	int clientAddrLen = sizeof(*client);	

	// printf("\nTEARDOWN\n");
	while(!eof_ack_flag && (retryCount < 9)) {
		if(pollCall(ONE_SEC) != TIMEOUT) {
			*pduLen = safeRecvfrom(childSocket, pduBuffer, MAX_PDU, 0, (struct sockaddr *) client, &clientAddrLen);
			retryCount = 0;
			if(!in_cksum((unsigned short*)pduBuffer, *pduLen))  {
				switch(pduBuffer->flag) {
				case FLAG_RR:
				processRR(pduBuffer);
				break;
				case FLAG_SREJ:
				processSREJ(pduBuffer, pduLen, childSocket, client);
				break;
				case FLAG_EOF_ACK:
				// printf("GOT EOF ACK FLAG");
				eof_ack_flag = 1;
				break;
				default:
				break;
				}
			}
		}
		
		else {
			retryCount++;
			*pduLen = getLowest(pduBuffer);

			if(pduBuffer->flag != FLAG_EOF)
				setFlag(pduBuffer, *pduLen, FLAG_TIMEOUT_DATA);
			safeSendto(childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
		}
	}
	removeFromPollSet(childSocket);
	teardownWindow();
	close(childSocket);
	fclose(*fd);
	freePollSet();
	return DONE;
}


void childFSM(pduPacket *pduBuffer, uint16_t *pduLen, struct sockaddr_in6 *client) {
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
			state = use(pduBuffer, pduLen, &fd, childSocket, client);
			break;
			case TEARDOWN:
			state = teardown(pduBuffer, pduLen, childSocket, client, &fd);
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
	