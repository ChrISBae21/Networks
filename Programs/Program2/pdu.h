#ifndef __PDU_H__
#define __PDU_H__

#include <stdint.h>

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize);
void buildChatHeader(uint8_t * dataBuffer, int lengthOfData, uint8_t flag);
void buildInitialPDU(uint8_t * dataBuffer, uint8_t *handleName, uint8_t handleLen, uint8_t flag);

#endif