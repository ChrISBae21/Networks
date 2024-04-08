#ifndef STD_LBRS
#define STD_LBRS
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

/* #include <net/ethernet.h> */

#define IP 0x0800
#define ARP 0x0806

void print_mac(uint8_t* mac_addr);
void print_eth(uint8_t *dest, uint8_t *src, uint16_t type);
uint16_t eth_hdr(uint8_t *pkt_data);