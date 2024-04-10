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



int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData) {
    uint8_t *pduBuffer;
    int bytesSent;
    uint16_t nLen, hLen;

    hLen = lengthOfData + 2;
    nLen = htons(lengthOfData + 2);
    pduBuffer = (uint8_t*) sCalloc(hLen, sizeof(uint8_t));

    memcpy(pduBuffer, &nLen, 2);
    memcpy(pduBuffer+2, dataBuffer, lengthOfData);
    bytesSent = safeSend(socketNumber, pduBuffer, hLen, 0);
    
    free(pduBuffer);
    return bytesSent;
}



int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize) {
    uint16_t pduHeader;         // Total PDU Length
    uint16_t bytesReceived;

    bytesReceived = safeRecv(clientSocket, (uint8_t*) &pduHeader, 2, MSG_WAITALL);

    if(bytesReceived == 0) {
        return 0;
    }
    
    pduHeader = ntohs(pduHeader);       

    if(bufferSize < pduHeader) {
        perror("buffer size is too small on the receiving side\n");
        exit(-1);
    }

    bytesReceived = safeRecv(clientSocket, dataBuffer, pduHeader, MSG_WAITALL);
    if(bytesReceived == 0) {
        return 0;
    }
    return bytesReceived;


}
