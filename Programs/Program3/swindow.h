
#ifndef __SWINDOW_H__
#define __SWINDOW_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "pdu.h"



typedef struct window {
    uint32_t lower;
    uint32_t upper;
    uint32_t current;
    uint32_t windowSize;
} Window;

typedef struct bufferItem {
    uint8_t valid;
    uint32_t hSeq;
    // uint32_t seq;
    uint16_t pduLen;
    pduPacket pduPacket;

} BufferItem;



uint32_t moveWindowCurrent();
void initBuffer(uint32_t bufferLen);
void addToBuffer(pduPacket *pdu, uint16_t pduLen, uint32_t sequenceNumber);
#endif
