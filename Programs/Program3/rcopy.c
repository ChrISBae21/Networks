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

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"


#define MAXBUF 80

static uint32_t sequenceNumber = 0;


void talkToServer(int socketNum, struct sockaddr_in6 * server);
int readFromStdin(char * buffer);
int checkArgs(int argc, char * argv[]);


int main (int argc, char *argv[]) {
	int socketNum = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int portNumber = 0;
	float err = 0;
	portNumber = checkArgs(argc, argv);
	err = atof(argv[5]);
	sendErr_init(err, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

	rcopyInit(&socketNum, &server)
	// socketNum = setupUdpClientToServer(&server, argv[2], portNumber);
	
	talkToServer(socketNum, &server);
	
	close(socketNum);

	return 0;
}

int rcopyInit(char* fName, int *socketNum, struct sockaddr_in6 *server, char* hostName, int portNumber) {
	*socketNum = setupUdpClientToServer(&server, hostName, portNumber);

}

void talkToServer(int socketNum, struct sockaddr_in6 * server) {
	int serverAddrLen = sizeof(struct sockaddr_in6);
	int dataLen = 0; 
	char buffer[MAXBUF+1];
	char payload[MAXBUF+1];
	
	buffer[0] = '\0';
	while (buffer[0] != '.') {
		dataLen = readFromStdin(payload);
		dataLen = createPDU((uint8_t *)buffer, 1, 1, (uint8_t *)payload, dataLen);
		printf("Sending: \n---------------------------------------\n");
		printPDU((uint8_t *)buffer, dataLen);
		printf("---------------------------------------\n");
		printf("\n");
		// printf("Sending: %s with len: %d\n", buffer,dataLen);
	
		safeSendto(socketNum, buffer, dataLen, 0, (struct sockaddr *) server, serverAddrLen);
	      
	}
}

int readFromStdin(char * buffer) {
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n') {
		aChar = getchar();
		if (aChar != '\n') {
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

int checkArgs(int argc, char * argv[]) {
    int portNumber = 0;
	
    /* check command line arguments  */
	if (argc != 7) {
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
	portNumber = atoi(argv[7]);	
	return portNumber;
}





