#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>

#include "safeUtil.h"
#include "handleTable.h"



typedef struct __attribute__((packed)) HND_TBL {
    uint8_t valid;          // Valid Handle
    uint8_t handle[101];    // Handle Name (101 to account for addition of a possible null terminator)
} HND_TBL;


// Handle Table Public Variables
static HND_TBL *handleTable;
static uint32_t max = 64;
static uint32_t serverSocket = 4;   // keeps track of the server's socket
static uint32_t numClients;

void setupHandleTable(uint32_t mainSocket) {
    handleTable = (HND_TBL*) sCalloc(max, sizeof(HND_TBL));   
    serverSocket = mainSocket;
    numClients = 0;
}



/*
* returns a 1 if the client handle already exists
* returns a 0 on successful addition of client handle
*/
uint8_t addHandle(uint8_t *handle, uint8_t handleLength, uint8_t socketNo) {
    uint32_t newMax = max;
    uint32_t temp;

    handle[handleLength] = '\0';

    // expand the list of sockets
    if(socketNo >= max) {
        while(newMax <= socketNo) {
            newMax *= 2;
        }
        newMax+=serverSocket;
        handleTable = srealloc(handleTable, newMax);
        // Initializes the new memory allocation valid bits to 0
        for(uint32_t i = max; i < newMax; i++) {
            (&handleTable[i])->valid = 0;
        }
        
        max = newMax;
    }
    
    // if the handle name exists, return 1
    if( (temp = getHandleToSocket(handle, handleLength)) > 0 ) {
        return 1;
    }
    (&handleTable[socketNo])->valid = 1;
    memcpy((&handleTable[socketNo])->handle, handle, handleLength+1);
    numClients++;
    return 0;

}

/*
* removeHandle:
* removes the handle based on socket number
* returns 1 on success, 0 if the handle/socket doesn't exist
*/
uint8_t removeHandle(uint8_t socket) {
    if((&handleTable[socket])->valid == 0) {
        return 0;
    }

    (&handleTable[socket])->valid = 0; 
    numClients--;
    return 1;
}

void teardownHandleTable() {
    free(handleTable);
}


uint32_t getNumClients() {
    return numClients;
}
/*
* getSocketToHandle
* returns a pointer to the handle name if found
* else returns NULL if it does not exist
*/
uint8_t* getSocketToHandle(uint8_t socketNo) {
    
    if(handleTable[socketNo].valid) {
        return (&handleTable[socketNo])->handle;
    }
    return NULL;
}



/*
* getHandleToSocket: returns the socket number for the handle name
* if the handle doesn't exist, returns 0
* ASSUMED HANDLES ARE NULL TERMINATED
*/
uint32_t getHandleToSocket(uint8_t* handle, uint8_t handleLen) {
    uint32_t compare;       // temp variable to compare handles
    uint32_t socket = 3;    // Sockets 0, 1, and 2 occupied by STDIN/OUT/ERR

    while(socket < max) {
        compare = strcmp((char*)&((&handleTable[socket])->handle), (char*)handle);     // compare handle names
        
        if( ((&handleTable[socket])->valid) && (compare == 0) )                  // check if the socket is valid and matches handle
            return socket;          
        socket++;
    }


    return 0;

}

void listHandleTable() {
    for(uint32_t i = 0; i < max; i++) {
        if(((&handleTable[i])->valid)) {
            fprintf(stdout, "Socket: %d, Handle: %s\n", i, (char*) &((&handleTable[i])->handle));
        }
    }
}

// int main() {
//     setupHandleTable(4);

//     uint8_t handle[8] = {'h', 'a', 'n', 'd', 'l', 'e', '1', '\0'};
//     uint8_t handle2[8] = {'h', 'a', 'n', 'd', 'l', 'e', '2', '\0'};
//     uint8_t done = addHandle(handle, 8, 4);
//     uint8_t done2 = addHandle(handle2, 8, 5);
//     printf("Handle1 Name: %d\n", done);
//     printf("Handle2 Name: %d\n", done2);

//     uint8_t *ptr;
//     ptr = getSocketToHandle(4);
//     printf("Handle from Socket: %s\n", ptr);
//     removeHandle(4);
//     ptr = getSocketToHandle(4);
//     if(ptr == NULL) printf("does not exist\n");
//     else printf("Handle from Socket: %s\n", ptr);

//     uint32_t socket;
//     socket = getHandleToSocket(handle, 8);

//     if(socket == 0) printf("no such handle\n");
//     else printf("found: %d\n", socket);

//     done = addHandle(handle, 8, 4);
//     ptr = getSocketToHandle(4);
//     printf("Handle from Socket: %s\n", ptr);

//     socket = getHandleToSocket(handle, 8);

//     if(socket == 0) printf("no such handle\n");
//     else printf("found: %d\n", socket);
// }