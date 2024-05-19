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


int createPDU(uint8_t *pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen) {
    unsigned short checksum;
    int pduLength = 7;
    uint32_t nSequenceNumber = htonl(sequenceNumber);   // network order sequence number
    memcpy(pduBuffer, &nSequenceNumber, 4);
    pduBuffer[4] = pduBuffer[5] = 0;                    // clear checksum field
    pduBuffer[6] = flag;                                // flag
    memcpy(pduBuffer+7, payload, payloadLen);           // add the payload
    pduLength += payloadLen;                            // length of the pduBuffer
    checksum = in_cksum((unsigned short *)pduBuffer, pduLength);          // calculate checksum
    memcpy(pduBuffer+4, &checksum, 2);
    return pduLength;
}


void printPDU(uint8_t * aPDU, int pduLength) {
    uint32_t nSequenceNumber, hSequenceNumber;
    uint8_t flag;
    if(in_cksum((unsigned short *)aPDU, pduLength) != 0) {
        printf("PDU is corrupted\n");
        return;
    }

    memcpy(&nSequenceNumber, aPDU, 4);
    hSequenceNumber = ntohl(nSequenceNumber);
    flag = aPDU[6];
    printf("Sequence Number: %d\n", hSequenceNumber);
    printf("Flag: %d\n", flag);
    printf("Payload: %s\n", aPDU+7);
    printf("Payload Length: %d\n", pduLength - 7);

}