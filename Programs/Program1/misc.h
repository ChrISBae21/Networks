#ifndef STD_LBRS
#define STD_LBRS
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#include <netinet/ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void print_ip(uint32_t ip);
void print_mac(uint8_t* mac_addr);