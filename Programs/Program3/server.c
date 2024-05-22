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

typedef enum {
	MAINSERVER,
	FILENAME,
	OPEN, 
	CLOSED,
	TEARDOWN,
	DONE
} STATE;


#define MAXBUF 1400
#define TIMEOUT -1
#define ONE_SEC 1000

void processClient(int socketNum, float err);
int checkArgs(int argc, char *argv[]);
void serverFSM(char* argv[], int mainServerSocket);
STATE mainserver(pduPacket *pduBuffer, int *pduLen, int mainServerSocket, struct sockaddr_in6 *client);
STATE filename(int *childSocket, pduPacket *pduBuffer, int *pduLen, int mainServerSocket, struct sockaddr_in6 *client, FILE **fd, uint32_t *serverSeqNum);

int main (int argc, char *argv[]) { 
	int socketNum = 0;				
	int portNumber = 0;
	float err = 0;

	portNumber = checkArgs(argc, argv);
	err = atof(argv[1]);
	sendErr_init(err, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	socketNum = udpServerSetup(portNumber);

	serverFSM(argv, socketNum);

	// processClient(socketNum, err);
	close(socketNum);
	// waitpid(-1)
	return 0;
}


STATE mainserver(pduPacket *pduBuffer, int *pduLen, int mainServerSocket, struct sockaddr_in6 *client) {
	int clientAddrLen = sizeof(client);	
	STATE nextState = MAINSERVER;
	int pid;

	pollCall(-1);
	*pduLen = safeRecvfrom(mainServerSocket, pduBuffer, MAX_PDU, 0, (struct sockaddr *) client, &clientAddrLen);

	pid = fork();

	/* debug */
	printf("forked\n");

	if(pid == -1) {
		printf("Error when forking\n");
	}
	/* child process */
	if(pid == 0) nextState = FILENAME;
	return nextState;

}

STATE filename(int *childSocket, pduPacket *pduBuffer, int *pduLen, int mainServerSocket, struct sockaddr_in6 *client, FILE **fd, uint32_t *serverSeqNum) {
	char fileName[101];
	uint8_t fnameAckPayload[1] = {1};
	uint16_t fileNameLen;

	fileNameLen = *pduLen - (PDU_HEADER_LEN + 6);
	memcpy(fileName, pduBuffer->payload+6, fileNameLen);
	*pduLen = createPDU((uint8_t *)pduBuffer, *serverSeqNum, FLAG_FILENAME_ACK, fnameAckPayload, 1);
	/* debug */
	printf("\tpduLen: %d\n", *pduLen);

	if((*fd = fopen(fileName, "rb")) == NULL) {
		/* debug */
		printf("file didn't exist\n");

		pduBuffer->payload[0] = 0;
		safeSendto(mainServerSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(struct sockaddr_in6));
		return DONE;
	}
	*childSocket = safeSocket();
	safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(struct sockaddr_in6));
	/* debug */
	printf("sent filename ack to rcopy\n");
	return OPEN;

}

void serverFSM(char* argv[], int mainServerSocket) {
	STATE state = MAINSERVER;
	int childSocket;
	struct sockaddr_in6 client;
	uint32_t serverSeqNum = 0;		
	pduPacket pduBuffer;
	int pduLen;
	FILE *fd;

	addToPollSet(mainServerSocket);

		while(state != DONE) {
		switch(state) {
			case MAINSERVER:
			/* debug */
			printf("MAINSERVER\n");
			state = mainserver(&pduBuffer, &pduLen, mainServerSocket, &client);
			break;
			case FILENAME:
			/* debug */
			printf("FILENAME\n");
			state = filename(&childSocket, &pduBuffer, &pduLen, mainServerSocket, &client, &fd, &serverSeqNum);
			break;
			case OPEN:
			state = DONE;
			// state = inorder();
			break;
			case CLOSED:
			state = DONE;
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

	/* debug */
	printf("done\n");
	removeFromPollSet(mainServerSocket);

}
void processClient(int socketNum, float err) {
	int dataLen = 0; 
	char buffer[MAXBUF + 1];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	buffer[0] = '\0';
	while (buffer[0] != '.') {

		
		dataLen = safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) &client, &clientAddrLen);
		printf("back to main\n");
		printf("Received message from client with ");
		printIPInfo(&client);
		// printf(" Len: %d \'%s\'\n", dataLen, buffer);
		printf("---------------------------------------\n");
		printPDU((uint8_t *)buffer, dataLen);
		printf("---------------------------------------\n");

		// just for fun send back to client number of bytes received
		// sprintf(buffer, "bytes: %d", dataLen);
		
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


