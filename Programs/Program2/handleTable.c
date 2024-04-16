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


void addHandle(uint8_t *handle, uint8_t handleLength, uint8_t socket);
void setupHandleTable();
void teardownHandleTable();
void removeHandle(uint8_t *handle, uint8_t handleLength);



// Handle Table Public Variables
static HND_TBL *handleTable;
static int maxFileDescriptor = 0;
static int currentPollSetSize = 0;
static int max = 10;

typedef struct __attribute__((packed)) HND_TBL {
    uint8_t valid;          // Valid Handle
    uint8_t handle[101];    // Handle Name (101 to account for addition of a possible null terminator)
} HND_TBL;


void setupHandleTable() {
    handleTable = (HND_TBL*) sCalloc(max, sizeof(HND_TBL));   
}

void addHandle(uint8_t *handle, uint8_t handleLength, uint8_t socket) {
    uint8_t flag = 1;
    HND_TBL *tempHandleTable = handleTable;
    uint8_t index = 0;



    if(currentPollSetSize == max) {
        max *= 2;
        handleTable = srealloc(handleTable, max);
    }

    while(flag) {
        if(!(tempHandleTable->valid)) {
            tempHandleTable->valid = 1;
            memcpy(tempHandleTable->handle, handle, handleLength);
            //(tempHandleTable->handle)[handleLength] = '\0';             // NULL terminate
            flag = 0;
        }
        index++;
        tempHandleTable += 1;       // next node
    }
    

}

void removeHandle(uint8_t *handle, uint8_t handleLength) {
    uint8_t index = 0;
    uint8_t flag = 1;
    HND_TBL *tempHandleTable = handleTable;

    while( (memcmp(tempHandleTable->handle, handle, handleLength) != 0) ) {
        tempHandleTable += 1;
    }

    tempHandleTable->valid = 0; 
}

void teardownHandleTable() {
    free(handleTable);
}


// both these two functions need to check if the valid bit is set
void getHandleName(uint8_t *handle, uint8_t handleLength) {
    HND_TBL *tempHandleTable = handleTable;

    while( (memcmp(tempHandleTable->handle, handle, handleLength) != 0) ) {
        tempHandleTable += 1;
    }
}

void getSocketNumber() {
    
}