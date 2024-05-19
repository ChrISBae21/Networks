
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

typedef struct window {
    uint32_t lower;
    uint32_t upper;
    uint32_t current;
    uint32_t windowSize;
} Window;



#endif
