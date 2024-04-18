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



void buildChatHeader(uint8_t * dataBuffer, int lengthOfData, uint8_t flag) {
    uint16_t nLen;
    nLen = htons(lengthOfData + 3);
    memcpy(dataBuffer, &nLen, 2);
    memcpy(dataBuffer+2, &flag, 1);
}

void buildEntirePDU(uint8_t * dataBuffer, uint8_t *srcHandle, uint8_t srcHandleLen, uint8_t uint8_t flag) {

	buildChatHeader(dataBuffer, handleLen, flag);
    dataBuffer += 3;    // increment pointer by 3 to offset the header
    memcpy(dataBuffer, &handleLen, 1);
    dataBuffer += 1;
    memcpy(dataBuffer, handleName, handleLen);

    if(flag > 1) {

    }


}




int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData) {
    int bytesSent;
    uint16_t hLen;

    hLen = lengthOfData + 3;

    bytesSent = safeSend(socketNumber, dataBuffer, hLen, 0);
    return bytesSent-3;
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


    if(bufferSize < pduHeader) {
        perror("buffer size is too small on the receiving side\n");
        exit(-1);
    }

    bytesReceived = safeRecv(clientSocket, dataBuffer, pduHeader-2, MSG_WAITALL);

    
    if(bytesReceived == 0) {
        return 0;
    }
    return bytesReceived;

}
