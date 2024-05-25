
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


void initWindow(uint32_t windowLen, uint16_t packetLen);
uint32_t moveWindowCurrent();
uint8_t getWindowStatus();
void teardownWindow();
uint32_t getPDUWindow(pduPacket *pdu, uint32_t sequenceNumber);
uint32_t getBufferSize();
uint16_t getPayloadSize();
void storePDUWindow(pduPacket *pdu, uint16_t pduLen, uint32_t sequenceNumber);
void slideWindow(uint32_t low);
uint16_t getLowest(pduPacket *pduBuffer);
uint8_t checkValidPDU(uint32_t sequenceNumber);
uint16_t getPDUFromBuffer(pduPacket *pdu, uint32_t sequenceNumber);
void removeFromBuffer(uint32_t sequenceNumber);
void teardownBuffer();


void initBuffer(uint32_t bufferLen, uint16_t packetLen);
void addToBuffer(pduPacket *pdu, uint16_t pduLen, uint32_t sequenceNumber);
#endif
