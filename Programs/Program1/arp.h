#ifndef STD_LBRS
#define STD_LBRS
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#define ARP_REQUEST 1
#define ARP_REPLY 2


typedef struct __attribute__((packed)) ARP_HDR {
    uint16_t opcode;                /* Opcode */
    uint8_t sender_mac[6];          /* Sender MAC */
    uint8_t target_mac[6];          /* Target MAC */
    uint32_t sender_ip;             /* Sender IP */
    uint32_t target_ip;             /* Target IP */
} ARP_HDR;


void arp_hdr(uint8_t *pkt_data);
void print_arp_hdr(ARP_HDR *arp_hdr);