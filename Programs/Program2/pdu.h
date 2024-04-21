#ifndef __PDU_H__
#define __PDU_H__

#include <stdint.h>

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize);
void buildPduPacket(uint8_t *dataBuffer, int lengthOfData, uint8_t flag);
uint8_t unpackPacketHandle(uint8_t *inHandle, uint8_t *outHandle);
uint8_t packFlagAndHandle(uint8_t *sendBuf, uint8_t handleLen, uint8_t *handleName, uint8_t flag);
#endif

#define PDU_HEADER_LEN 2