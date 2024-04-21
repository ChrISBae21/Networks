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
#include <errno.h>

#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"



/*
* Adds
*/
uint8_t packFlagAndHandle(uint8_t *sendBuf, uint8_t handleLen, uint8_t *handleName, uint8_t flag) {
	memcpy(sendBuf++, &flag, 1);				// Flag byte
	memcpy(sendBuf++, &handleLen, 1);			// Source Handle Length
	memcpy(sendBuf, handleName, handleLen);		// Handle Name
	return handleLen + 2;						// Total length
}


uint8_t unpackPacketHandle(uint8_t *inHandle, uint8_t *outHandle) {
	uint8_t len;
	len = *inHandle++;
	memcpy(outHandle, inHandle, len);
	outHandle[len] = '\0';
	return len;
}


int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData) {
    int bytesSent;
    uint16_t hLen;
    uint16_t nLen;
    uint8_t pduPacket[MAXBUF];

    nLen = htons(lengthOfData + PDU_HEADER_LEN);
    memcpy(pduPacket, &nLen, PDU_HEADER_LEN);
    memcpy(pduPacket+PDU_HEADER_LEN, dataBuffer, lengthOfData);

    hLen = lengthOfData + PDU_HEADER_LEN;

    bytesSent = safeSend(socketNumber, pduPacket, hLen, 0);
    
    return (bytesSent - PDU_HEADER_LEN);
}


/* returns the number of bytes received */
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize) {
    uint16_t pduHeader;         // Total PDU Length
    uint16_t bytesReceived;

    bytesReceived = safeRecv(clientSocket, (uint8_t*) &pduHeader, 2, MSG_WAITALL);

    if(bytesReceived == 0) {
        return 0;
    }

    pduHeader = ntohs(pduHeader);       

    
    
    if(bufferSize < pduHeader-2) {
        perror("buffer size is too small on the receiving side\n");
        exit(-1);
    }

    bytesReceived = safeRecv(clientSocket, dataBuffer, pduHeader-2, MSG_WAITALL);

    
    if(bytesReceived == 0) {
        return 0;
    }
    return bytesReceived;

}
