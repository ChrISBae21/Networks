#include "swindow.h"
#include "safeUtil.h"


// when buffering for rcopy, the index is just (seq no % window size)

static Window *sWindow;
static uint8_t *buffer;


/* creates a buffer of length bufferLen */
void initBuffer(uint32_t bufferLen) {

}

/* initializes a the window with size windowLen */
void initWindow(uint32_t windowLen) {
    sWindow = sCalloc(1, sizeof(Window));
    sWindow->upper = windowLen;
    sWindow->windowSize = windowLen;
}

/* slides the window as RR's are sent */
void slideWindow(uint32_t low) {
    sWindow->lower = low;
    sWindow->upper = low + sWindow->windowSize;
}

/* checks if the window is open (1) or closed (0) */
uint8_t getWindowStatus() {
    return (sWindow->current < sWindow->upper);
}

void teardownWindow() {
    free(sWindow);
}
