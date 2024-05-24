
#ifndef __PDU_H__
#define __PDU_H__

#define FLAG_RR             5
#define FLAG_SREJ           6
#define FLAG_FILENAME       8
#define FLAG_FILENAME_ACK   9
#define FLAG_EOF            10
#define FLAG_DATA           16
#define FLAG_SREJ_DATA      17
#define FLAG_TIMEOUT_DATA   18

#define MAX_PAYLOAD 1400
#define PDU_HEADER_LEN 7
#define MAX_PDU (MAX_PAYLOAD + PDU_HEADER_LEN)

typedef struct packet {
    uint32_t nSeqNo;
    uint16_t checksum;
    uint8_t flag;
    uint8_t payload[MAX_PAYLOAD];
} pduPacket;

int createPDU(pduPacket *pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen);
void printPDU(pduPacket *aPDU, int pduLength);
uint32_t getHSeqNum(pduPacket *pduBuffer);
uint8_t getFlag(pduPacket *pduBuffer);
void getFromFileName(pduPacket *pduBuffer, int pduLen, char *fileName);
void setFlag(pduPacket *pduBuffer, int pduLen, uint8_t flag);
uint16_t getWindowSizeFromPDU(pduPacket *pduBuffer);
uint32_t getPayloadSizeFromPDU(pduPacket *pduBuffer);





#endif