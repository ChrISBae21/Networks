#ifndef STD_LBRS
#define STD_LBRS
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#define UDP 17
#define ICMP 1
#define TCP 6

// common TCP/UDP ports
#define HTTP 80
#define TELNET 23
#define FTP 21
#define POP3 110
#define SMTP 25
#define DNS 53

// ICMP Reply Request
#define REQUEST 8
#define REPLY 0

typedef struct __attribute__((packed)) IP_HDR {
    uint8_t *ip_mem_addr;           // memory address of the start of the IP Header
    uint8_t ver;                    // Version 
    uint8_t len;                    // Length
    uint8_t dscp_ecn;               // DSCP and ECN
    uint8_t ttl;                    // Time to Live
    uint8_t protocol;               // Protocol Field
    uint16_t checksum;              // Checksum
    uint32_t src_ip;                // Source IP
    uint32_t dest_ip;               // Destination IP
} IP_HDR;

void print_ip_hdr(IP_HDR *ip_hdr);
IP_HDR* ip(uint8_t *pkt_data);
void icmp_tcp_udp(uint8_t protocol, uint8_t *pkt_data);
void udp(uint8_t *pkt_data);
void tcp(uint8_t *pkt_data);
void icmp(uint8_t *pkt_data);
void print_port(uint16_t port, uint8_t protocol);
