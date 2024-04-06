#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ethernet.h"
/*
Prints formatted MAC address
*/
void print_mac(uint8_t* mac_addr) {
    uint8_t* ptr;
    ptr = (uint8_t*) ether_ntoa((struct ether_addr*) mac_addr);
    printf("%s\n", ptr);

}


/*
Prints the Ethernet header
*/
void print_eth(uint8_t *dest, uint8_t *src, uint16_t type) {
    printf("\tEthernet Header\n");
    printf("\t\tDest Mac: ");
    print_mac(dest);
    printf("\t\tSource Mac: ");
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

uint16_t ethernet(uint8_t *pkt_data) {
    // always has dest, src, and type
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;

    memcpy(dest, pkt_data, 6);
    memcpy(src, (pkt_data+6), 6);
    memcpy(&type, (pkt_data+12), 2);

    print_eth(dest, src, type);

    return type;
    
}