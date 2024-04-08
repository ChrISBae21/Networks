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

/* common TCP/UDP ports */
#define HTTP 80
#define TELNET 23
#define FTP 21
#define POP3 110
#define SMTP 25
#define DNS 53

#define TCP 6

void print_ip(uint32_t ip);
void print_mac(uint8_t* mac_addr);
void print_port(uint16_t port, uint8_t protocol);