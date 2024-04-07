#ifndef STD_LBRS
#define STD_LBRS
#include <stdint.h>
#endif

#define UDP 17
#define ICMP 1
#define TCP 6

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

void print_ip(IP_HDR *ip_hdr);
IP_HDR* ip(uint8_t *pkt_data);