
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "checksum.h"
#include "ip.h"
#include "misc.h"


void print_ip_hdr(IP_HDR *ip_hdr) {
    

    printf("\t\tIP Version: %d\n", ip_hdr->ver);
    printf("\t\tHeader Len (bytes): %d\n", ip_hdr->len);
    printf("\t\tTOS subfields:\n");
    printf("\t\t\tDiffserv bits: %d\n", (ip_hdr->dscp_ecn & 0xF0) / 16);
    printf("\t\t\tECN bits: %d\n", (ip_hdr->dscp_ecn & 0x0F));
    printf("\t\tTTL: %d\n", ip_hdr->ttl);
    printf("\t\tProtocol: ");
    switch(ip_hdr->protocol) {
        case UDP: printf("UDP\n");
            break;
        case ICMP: printf("ICMP\n");
            break;
        case TCP: printf("TCP\n");
            break;
        default: printf("Unknown\n");
            break;
    }

    if(in_cksum((unsigned short*) ip_hdr->ip_mem_addr, ip_hdr->len) == 0) 
        printf("\t\tChecksum: Correct (0x%x)\n", ip_hdr->checksum);
    else 
        printf("\t\tChecksum: Incorrect (0x%x)\n", ip_hdr->checksum);

    printf("\t\tSender IP: ");
    print_ip(ip_hdr->src_ip);
    printf("\t\tDest IP: ");
    print_ip(ip_hdr->dest_ip);
    printf("\n");
}

IP_HDR* ip(uint8_t *pkt_data) {
    uint8_t ver_len;                // Version and Length
    uint16_t checksum;              // Checksum

    IP_HDR *ip_hdr = malloc(sizeof(IP_HDR));
    ip_hdr->ip_mem_addr = pkt_data;             

    memcpy(&ver_len, pkt_data, 1);              // grab ver and len byte
    ip_hdr->ver = ((ver_len & 0xF0) / 16);      // set Version
    ip_hdr->len = (ver_len & 0x0F) * 4;         // set length
    pkt_data+=1;                                // move 1 byte

    memcpy(&(ip_hdr->dscp_ecn), pkt_data, 1);   // grab dscp and ecn byte
    pkt_data+=7;                                // move 7 bytes

    memcpy(&(ip_hdr->ttl), pkt_data, 1);        // grab ttl byte
    pkt_data+=1;                                // move 1 byte

    ip_hdr->protocol = pkt_data[0];             // procotol field
    pkt_data+=1;                                // move 1 byte

    memcpy(&checksum, pkt_data, 2);             // grab checksum (2 bytes)
    ip_hdr->checksum = ntohs(checksum);         // get checksum in little endian
    pkt_data+=2;                                // move 2 bytes

    memcpy(&(ip_hdr->src_ip), pkt_data, 4);     // grab source IP
    memcpy(&(ip_hdr->dest_ip), pkt_data+4, 4);  // grab destination IP
    print_ip_hdr(ip_hdr);                           // print the IP header
    return ip_hdr;
}