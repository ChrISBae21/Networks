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


/* creates a buffer of length bufferLen */
void initBuffer(uint32_t bufferLen) {
    buffSize = bufferLen;
    buffer = sCalloc(bufferLen, sizeof(BufferItem));
}

/* adds to the buffer */
void addToBuffer(pduPacket *pdu, uint8_t pduLen, uint32_t sequenceNumber) {
    uint32_t index = sequenceNumber % buffSize;
    buffer[index].seq = sequenceNumber;
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
void initWindow(uint32_t windowLen) {
    sWindow = sCalloc(1, sizeof(Window));
    sWindow->upper = windowLen;
    sWindow->windowSize = windowLen;

    initBuffer(windowLen);
}

/* adds a PDU in the window */
void storePDUWindow(pduPacket *pdu, uint32_t pduLen, uint32_t sequenceNumber) {
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

/* checks if the window is open (1) or closed (0) */
uint8_t getWindowStatus() {
    return (sWindow->current < sWindow->upper);
}

/* frees the window */
void teardownWindow() {
    free(sWindow);
}
