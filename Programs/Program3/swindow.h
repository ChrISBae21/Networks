
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

#define PDU_HEADER 7
#define MAX_PAYLOAD 1400
#define MAX_PDU (MAX_PAYLOAD + PDU_HEADER)

typedef struct window {
    uint32_t lower;
    uint32_t upper;
    uint32_t current;
    uint32_t windowSize;
} Window;

typedef struct bufferItem {
    uint32_t seq;
    uint32_t buffSize;
    uint32_t pduLen;
    uint8_t valid;
    uint8_t pdu[MAX_PDU]

} BufferItem;


#endif
