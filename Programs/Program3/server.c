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

typedef enum {
	START,
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
void childFSM(pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client);
void mainserver(int mainServerSocket, float err);
STATE filename(int *childSocket, pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client, FILE **fd, uint32_t *serverSeqNum);
void handleZombies(int sig);

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

STATE filename(int *childSocket, pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client, FILE **fd, uint32_t *serverSeqNum) {
	char fileName[101];
	uint8_t fnameAckPayload = 0;


	if(in_cksum((unsigned short*)pduBuffer, *pduLen))  {
		return DONE;
	}
	/* grab the file name */

	getFromFileName(pduBuffer, *pduLen, fileName);
	
	/* create a new socket */
	*childSocket = safeSocket();

	/* check if the file exists */
	if((*fd = fopen(fileName, "rb")) == NULL) {

		*pduLen = createPDU((uint8_t *)pduBuffer, *serverSeqNum, FLAG_FILENAME_ACK, &fnameAckPayload, 1);
		safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
		return DONE;
	}

	fnameAckPayload = 1;
	*pduLen = createPDU((uint8_t *)pduBuffer, (*serverSeqNum)++, FLAG_FILENAME_ACK, &fnameAckPayload, 1);
	
	/* DEBUG */
	// *pduLen = createPDU((uint8_t *)pduBuffer, 0, FLAG_FILENAME_ACK, &fnameAckPayload, 1);
	// safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));

	// *pduLen = createPDU((uint8_t *)pduBuffer, 1, FLAG_DATA, &fnameAckPayload, 1);
	// safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));

	// *pduLen = createPDU((uint8_t *)pduBuffer, 2, FLAG_DATA, &fnameAckPayload, 1);
	
	/* sends a good file name packet */
	safeSendto(*childSocket, pduBuffer, *pduLen, 0, (struct sockaddr *) client, sizeof(*client));
	return OPEN;

}

void childFSM(pduPacket *pduBuffer, int *pduLen, struct sockaddr_in6 *client) {
	STATE state = START;
	int childSocket;
	uint32_t serverSeqNum = 0;
	FILE *fd;

		while(state != DONE) {
			switch(state) {
				case START:
				state = FILENAME;
				break;
				case FILENAME:
				state = filename(&childSocket, pduBuffer, pduLen, client, &fd, &serverSeqNum);
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
	