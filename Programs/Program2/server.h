#ifndef __SERVER_H__
#define __SERVER_H__

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
#include <ctype.h>
#endif

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void addNewSocket(int mainServerSocket);
void processClient(int clientSocket, uint8_t msgLen, uint8_t *dataBuf);
void serverControl(int mainServerSocket);
void unpackMessagePacket(int srcClientSocket, uint8_t inputBufLen, uint8_t *inputBuf);
void sendClientList(int clientSocket, uint8_t *sendBuf);
void sendBroadcast(int clientSocket, uint8_t sendLen, uint8_t *sendBuf);