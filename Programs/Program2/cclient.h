
#ifndef __CCLIENT_H__
#define __CCLIENT_H__

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


void sendToServer(int socketNum, uint8_t *sendBuf, uint8_t sendLen);
uint16_t readFromStdin(uint8_t * buffer);
void checkArgs(int argc, char * argv[]);
void clientControl(int mainServerSocket, uint8_t *handleName, uint8_t handleLen);
void unpackPacket(int mainServerSocket);
void initialPacket(int mainServerSocket, uint8_t *handleName);
uint16_t processStdin(uint8_t *stdinBuf, uint16_t stdinLen, uint8_t srcHandleLen, uint8_t *srcHandleName, uint8_t socket);
uint16_t getDestHandles(uint8_t *handleBuf, uint8_t **stdinBuf, uint8_t numHandles, uint16_t *stdinLen);
void processServerPacket(int mainServerSocket, uint16_t msgLen, uint8_t *inputBuf);
void closeClient(int mainServerSocket);
void packPacket(int mainServerSocket, uint8_t *handleName, uint8_t handleLen);
uint16_t packMessagePacket(uint8_t *stdinBuf, uint16_t stdinLen, uint8_t numDestHandles, uint8_t srcHandleLen, uint8_t *srcHandleName, uint8_t flag, uint8_t socket, uint8_t broadcast);
void fragmentMsg(uint16_t msgLen, uint8_t *msg, uint8_t *pckMsg, uint8_t *payload, uint16_t payloadLen, int8_t socket);
void listClients(int mainServerSocket, uint16_t msgLen, uint8_t *inputBuf);
void unpackMessagePacket(uint8_t *inputBuf, uint8_t broadcast);
int verifyRecv(int mainServerSocket, uint8_t *dataBuffer, int bufferSize);