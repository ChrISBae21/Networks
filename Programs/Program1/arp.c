#include <arpa/inet.h>
#include "arp.h"
#include "misc.h"

void print_arp_hdr(ARP_HDR *arp_hdr) {
    printf("\t\tOpcode: ");
    switch(ntohs(arp_hdr->opcode)) {
        case ARP_REQUEST: printf("Request\n");
            break;
        case ARP_REPLY: printf("Reply\n");
            break;
        default: printf("Unknown\n");
    }

    printf("\t\tSender MAC: ");
    print_mac(arp_hdr->sender_mac);
    printf("\t\tSender IP: ");
    print_ip(arp_hdr->sender_ip);
    printf("\t\tTarget MAC: ");
    print_mac(arp_hdr->target_mac);
    printf("\t\tTarget IP: ");
    print_ip(arp_hdr->target_ip);
    printf("\n");

}

void arp_hdr(uint8_t *pkt_data) {
    pkt_data+=6;                        /* skip to opcode */
    ARP_HDR *arp_hdr = malloc(sizeof(ARP_HDR));
    memcpy(&(arp_hdr->opcode), pkt_data, 2);
    pkt_data+=2;
    memcpy(arp_hdr->sender_mac, pkt_data, 6);
    pkt_data+=6;
    memcpy(&(arp_hdr->sender_ip), pkt_data, 4);
    pkt_data+=4;
    memcpy(arp_hdr->target_mac, pkt_data, 6);
    pkt_data+=6;
    memcpy(&(arp_hdr->target_ip), pkt_data, 4);
    print_arp_hdr(arp_hdr);
    free(arp_hdr);

}