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

// Handle Table Public Variables
static struct HND_TBL *handleTable;
static int maxFileDescriptor = 0;
static int currentPollSetSize = 0;

typedef struct __attribute__((packed)) HND_TBL {
    uint8_t valid;          // Valid Handle
    uint8_t socketNo;       // Socket Number
    uint8_t *handle;        // Handle Name
} HND_TBL;



void setupHandleTable() {
    handleTable = (HND_TBL*) sCalloc(10, sizeof(HND_TBL));

}