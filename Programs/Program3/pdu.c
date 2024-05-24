#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "cpe464.h"
#include "pdu.h"


int createPDU(pduPacket *pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen) {
    unsigned short checksum;
    int pduLength = 7;
    uint32_t nSequenceNumber = htonl(sequenceNumber);               // network order sequence number
    pduBuffer->nSeqNo = nSequenceNumber;
    pduBuffer->checksum = 0;
    pduBuffer->flag = flag;
    memcpy(pduBuffer->payload, payload, payloadLen);                // add the payload
    pduLength += payloadLen;                                        // length of the pduBuffer
    checksum = in_cksum((unsigned short *)pduBuffer, pduLength);    // calculate checksum
    pduBuffer->checksum = checksum;
    return pduLength;
}


void printPDU(pduPacket *aPDU, int pduLength) {
    uint32_t nSequenceNumber, hSequenceNumber;
    uint8_t flag;
    if(in_cksum((unsigned short *)aPDU, pduLength) != 0) {
        printf("PDU is corrupted\n");
        return;
    }

    memcpy(&nSequenceNumber, aPDU, 4);
    hSequenceNumber = ntohl(nSequenceNumber);
    flag = aPDU->flag;
    printf("Sequence Number: %d\n", hSequenceNumber);
    printf("Flag: %d\n", flag);
    printf("Payload: %s\n", aPDU->payload);
    printf("Payload Length: %d\n", pduLength - 7);

}

uint32_t getHSeqNum(pduPacket *pduBuffer) {
    return ntohl(pduBuffer->nSeqNo);
}

uint8_t getFlag(pduPacket *pduBuffer) {
    return pduBuffer->flag;
}

void getFromFileName(pduPacket *pduBuffer, int pduLen, char *fileName) {
    uint16_t fileNameLen;
    fileNameLen = pduLen - (PDU_HEADER_LEN + 6);
	memcpy(fileName, pduBuffer->payload+6, fileNameLen);
	fileName[fileNameLen] = '\0';
}

void setFlag(pduPacket *pduBuffer, int pduLen, uint8_t flag) {
    unsigned short checksum;

    pduBuffer->flag = flag;
    pduBuffer->checksum = 0;
    checksum = in_cksum((unsigned short *)pduBuffer, pduLen);          // calculate checksum
    pduBuffer->checksum = checksum;

}



uint32_t getWindowSizeFromPDU(pduPacket *pduBuffer) {
    uint32_t windowSize;
    memcpy(&windowSize, pduBuffer->payload+2, 4);
    /* debug */
	// printf("\nWindow Size: %d\n", windowSize);
    return windowSize;
}

uint16_t getPayloadSizeFromPDU(pduPacket *pduBuffer) {
    uint16_t payloadSize;
    /* debug */
	// printf("\nBefore Payload Size\n");

    memcpy(&payloadSize, pduBuffer->payload, 2);
    /* debug */
	// printf("\nPayload Size: %d\n", payloadSize);
    return payloadSize;
}