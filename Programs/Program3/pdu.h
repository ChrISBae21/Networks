
#ifndef __PDU_H__
#define __PDU_H__
int createPDU(uint8_t *pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen);
void printPDU(uint8_t * aPDU, int pduLength); 

#define FLAG_RR             5
#define FLAG_SREJ           6
#define FLAG_FILENAME       8
#define FLAG_FILENAME_ACK   9
#define FLAG_EOF            10
#define FLAG_DATA           16
#define FLAG_SREJ_DATA      17
#define FLAG_TIMEOUT_DATA   18
#endif