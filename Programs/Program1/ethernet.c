
#include <arpa/inet.h>
#include "ethernet.h"
#include "misc.h"


/*
Prints the Ethernet header
*/
void print_eth(uint8_t *dest, uint8_t *src, uint16_t type) {
    printf("\tEthernet Header\n");
    printf("\t\tDest MAC: ");
    print_mac(dest);
    printf("\t\tSource MAC: ");
    print_mac(src);

    printf("\t\tType: ");
    switch(ntohs(type)) {
        case IP: 
            printf("IP");
            break;
        case ARP:
            printf("ARP");
            break;
        default:
            printf("Unknown");
                
    }

    printf("\n\n");
}

uint16_t eth_hdr(uint8_t *pkt_data) {
    /* always has dest, src, and type */
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;

    memcpy(dest, pkt_data, 6);
    memcpy(src, (pkt_data+6), 6);
    memcpy(&type, (pkt_data+12), 2);

    print_eth(dest, src, type);

    return type;
    
}