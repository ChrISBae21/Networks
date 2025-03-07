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

/* ICMP Reply Request */
#define REQUEST 8 
#define REPLY 0

typedef struct __attribute__((packed)) IP_HDR {
    uint8_t *ip_mem_addr;           /* memory address of the start of the IP Header */
    uint8_t ver;                    /* Version */
    uint8_t len;                    /* Length */
    uint8_t dscp_ecn;               /* DSCP and ECN */
    uint8_t ttl;                    /* Time to Live */
    uint8_t protocol;               /* Protocol Field */
    uint16_t checksum;              /* Checksum */
    uint32_t src_ip;                /* Source IP */
    uint32_t dest_ip;               /* Destination IP */
    uint16_t tot_len;               /* Total length from IP header on */
} IP_HDR;

typedef struct __attribute__((packed)) TCP_HDR {
    uint8_t *tcp_mem_addr;          /* memory address of the start of the TCP Header */
    uint16_t src_port;              /* Source Port */
    uint16_t dest_port;             /* Destination Port */
    uint32_t seq_num;               /* Sequence Number */
    uint32_t ack_num;               /* Acknowledge Number */
    uint8_t data_offset;            /* Data Offset */
    uint8_t flag;                   /* Flags */
    uint16_t win_size;              /* Window Size */
    uint16_t checksum;              /* Checksum */
} TCP_HDR;

/*IP */
IP_HDR* ip(uint8_t *pkt_data);
void print_ip_hdr(IP_HDR *ip_hdr);

/*ICMP, TCP, and UDP */
void icmp_tcp_udp(uint8_t protocol, uint8_t *pkt_data, IP_HDR *ip_hdr);

/*UDP */
void udp(uint8_t *pkt_data);

/*TCP */
void tcp(uint8_t *pkt_data, IP_HDR *ip_hdr);
void print_tcp(TCP_HDR *tcp_hdr, uint8_t *pseudo_hdr, uint16_t len);
void print_tcp_flag(uint8_t flag);
uint8_t* mk_pseudo_hdr(uint32_t src, uint32_t dest, uint8_t protocol, uint16_t tcp_len);


void icmp(uint8_t *pkt_data);
void print_port(uint16_t port, uint8_t protocol);
