#include <arpa/inet.h>
#include "arp.h"
#include "misc.h"

void print_arp_hdr(ARP_HDR *arp_hdr) {
    fprintf(stdout, "\t\tOpcode: ");
    switch(ntohs(arp_hdr->opcode)) {
        case ARP_REQUEST: fprintf(stdout, "Request\n");
            break;
        case ARP_REPLY: fprintf(stdout, "Reply\n");
            break;
        default: fprintf(stdout, "Unknown\n");
    }

    fprintf(stdout, "\t\tSender MAC: ");
    print_mac(arp_hdr->sender_mac);
    fprintf(stdout, "\t\tSender IP: ");
    print_ip(arp_hdr->sender_ip);
    fprintf(stdout, "\t\tTarget MAC: ");
    print_mac(arp_hdr->target_mac);
    fprintf(stdout, "\t\tTarget IP: ");
    print_ip(arp_hdr->target_ip);
    fprintf(stdout, "\n");

}

void arp_hdr(uint8_t *pkt_data) {
    ARP_HDR *arp_hdr = malloc(sizeof(ARP_HDR));
    if(arp_hdr == NULL) {
        perror("ARP Header Malloc error\n");
        exit(-1);
    }
    pkt_data+=6;                        /* skip to opcode */
    memcpy(&(arp_hdr->opcode), pkt_data, 2);    /* opcode */
    pkt_data+=2;
    memcpy(arp_hdr->sender_mac, pkt_data, 6);   /* sender MAC*/
    pkt_data+=6;
    memcpy(&(arp_hdr->sender_ip), pkt_data, 4); /* sender IP*/
    pkt_data+=4;
    memcpy(arp_hdr->target_mac, pkt_data, 6);   /* target MAC*/
    pkt_data+=6;
    memcpy(&(arp_hdr->target_ip), pkt_data, 4); /* target IP*/
    print_arp_hdr(arp_hdr);
    free(arp_hdr);

}