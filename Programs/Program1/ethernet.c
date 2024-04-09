
#include <arpa/inet.h>
#include "ethernet.h"
#include "misc.h"


/*
Prints the Ethernet header
*/
void print_eth(uint8_t *dest, uint8_t *src, uint16_t type) {
    fprintf(stdout, "\tEthernet Header\n");
    fprintf(stdout, "\t\tDest MAC: ");
    print_mac(dest);
    fprintf(stdout, "\t\tSource MAC: ");
    print_mac(src);

    fprintf(stdout, "\t\tType: ");
    switch(ntohs(type)) {
        case IP: 
            fprintf(stdout, "IP");
            break;
        case ARP:
            fprintf(stdout, "ARP");
            break;
        default:
            fprintf(stdout, "Unknown");
                
    }

    fprintf(stdout, "\n\n");
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