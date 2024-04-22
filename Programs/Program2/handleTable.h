
#ifndef __HANDLE_TABLE_H__
#define __HANDLE_TABLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif


uint8_t addHandle(uint8_t *handle, uint8_t handleLength, uint8_t socket);
void setupHandleTable(uint32_t mainSocket);
void teardownHandleTable();
uint8_t removeHandle(uint32_t socket);
uint8_t getSocketToHandle(uint32_t socketNo, uint8_t *handle);
uint32_t getHandleToSocket(uint8_t* handle, uint8_t handleLen);
uint32_t getNumClients();