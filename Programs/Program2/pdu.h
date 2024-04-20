#ifndef __PDU_H__
#define __PDU_H__

#include <stdint.h>

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize);
void buildPduPacket(uint8_t *dataBuffer, int lengthOfData, uint8_t flag);
#endif

#define PDU_HEADER_LEN 2