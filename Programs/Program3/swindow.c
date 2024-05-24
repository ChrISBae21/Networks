#include "swindow.h"
#include "safeUtil.h"
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

static Window *sWindow;
static BufferItem *buffer;
uint32_t buffSize;
uint16_t packetSize;

/* The Buffer Struct contains host-order sequence number. PDU struct contains network-order sequence number*/

/* creates a buffer of length bufferLen */
void initBuffer(uint32_t bufferLen, uint16_t packetLen) {
    buffSize = bufferLen;
    packetSize = packetLen;
    buffer = sCalloc(bufferLen, sizeof(BufferItem));
}

/* adds to the buffer sequenceNumber is in host-order */
void addToBuffer(pduPacket *pdu, uint16_t pduLen, uint32_t sequenceNumber) {
    // uint32_t hseqNo = ntohl(sequenceNumber);
    // uint32_t index = hseqNo % buffSize;
    uint32_t index = sequenceNumber % buffSize;
/* debug */
	printf("\nIndex: %d\nCurrent: %d\n", index, sWindow->current);

    // buffer[index].hSeq = hseqNo;
    buffer[index].hSeq = sequenceNumber;
    buffer[index].valid = 1;
    buffer[index].pduLen = pduLen;
    memcpy(&(buffer[index].pduPacket), pdu, pduLen);
}

/* removes from the buffer */
void removeFromBuffer(uint32_t sequenceNumber) {
    uint32_t index = sequenceNumber % buffSize;
    buffer[index].valid = 0;
}

/* gets the pdu from the buffer */
uint32_t getPDUFromBuffer(pduPacket *pdu, uint32_t sequenceNumber) {
    uint32_t index = sequenceNumber % buffSize;
    memcpy(pdu, &(buffer[index].pduPacket), buffer[index].pduLen);
    return buffer[index].pduLen;
}

uint32_t getBufferSize() {
    return buffSize;
}
uint16_t getPayloadSize() {
    return packetSize;
}

/* checks if the pdu is valid */
uint8_t checkValidPDU(uint32_t sequenceNumber) {
    uint32_t index = sequenceNumber % buffSize;
    return buffer[index].valid;
}

/* frees the buffer */
void teardownBuffer() {
    free(buffer);
}



/* initializes a the window with size windowLen */
void initWindow(uint32_t windowLen, uint16_t packetLen) {
    sWindow = sCalloc(1, sizeof(Window));
    sWindow->current = 1;
    sWindow->lower = 1;
    sWindow->upper = windowLen + 1;
    sWindow->windowSize = windowLen;

    initBuffer(windowLen, packetLen);
}

/* adds a PDU in the window, sequencenumber is in host-order */
void storePDUWindow(pduPacket *pdu, uint16_t pduLen, uint32_t sequenceNumber) {
    /* debug */
	// printf("\nSeq Num: %d\nCurrent: %d\n", sequenceNumber, sWindow->current);

    if(sequenceNumber != sWindow->current) {
        printf("ERROR IN storePDUWindow()\n");
        exit(-1);
    }
    addToBuffer(pdu, pduLen, sWindow->current);
    moveWindowCurrent();
}

/* gets the PDU within the window */
uint32_t getPDUWindow(pduPacket *pdu, uint32_t sequenceNumber) {
    
    if(sequenceNumber < sWindow->lower) {
        printf("ERROR IN getPDUWindow\n");
        exit(-1);
    }
    return getPDUFromBuffer(pdu, sequenceNumber);
}

/* returns the incremented value of current */
uint32_t moveWindowCurrent() {
    return ++sWindow->current;
}

/* slides the window as RR's are sent */
void slideWindow(uint32_t low) {
    // maybe check if current < new low
    sWindow->lower = low;
    sWindow->upper = low + sWindow->windowSize;
}

int getLowest(pduPacket *pduBuffer) {
    /* debug */

    return getPDUFromBuffer(pduBuffer, sWindow->lower);
}

/* checks if the window is open (1) or closed (0) */
uint8_t getWindowStatus() {
    return (sWindow->current < sWindow->upper);
}

/* frees the window */
void teardownWindow() {
    free(sWindow);
}
