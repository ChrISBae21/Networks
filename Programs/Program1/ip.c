
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


void icmp_tcp_udp(uint8_t protocol, uint8_t *pkt_data) {
    switch(protocol) {
        case UDP: 
            printf("\tUDP Header\n");
            udp(pkt_data);
            break;
        case ICMP: 
            printf("\tICMP Header\n");
            icmp(pkt_data);
            break;
        case TCP:
            printf("\tTCP Header\n");
            tcp(pkt_data);
            break;
        default: printf("\tUnknown\n");
            break;
    }
    printf("\n");
}


void udp(uint8_t *pkt_data) {
    uint16_t src_port, dest_port;

    printf("\t\tSource Port: ");
    memcpy(&src_port, pkt_data, 2);
    print_port(ntohs(src_port), UDP);
    printf("\t\tDest Port: ");
    memcpy(&dest_port, pkt_data+2, 2);
    print_port(ntohs(dest_port), UDP);
}


void print_tcp_flag(uint16_t flag) {
    printf("\t\tFlag: \n");
}

void print_tcp(TCP_HDR *tcp_hdr) {
    printf("\t\tSource Port: ");
    print_port(ntohs(tcp_hdr->src_port), TCP);
    printf("\t\tDest Port: ");
    print_port(ntohs(tcp_hdr->dest_port), TCP);
    printf("\t\tSequence Number: %d\n", ntohl(tcp_hdr->seq_num));
    printf("\t\tACK Number: %d\n", ntohl(tcp_hdr->ack_num));
    printf("\t\tData Offset (bytes): %d\n", tcp_hdr->data_offset);

    print_tcp_flag(ntohs(tcp_hdr->flag));

    printf("\t\tWindow Size: %d\n", ntohs(tcp_hdr->win_size));
    if(in_cksum((unsigned short*) tcp_hdr->tcp_mem_addr, tcp_hdr->data_offset) == 0) 
        printf("\t\tChecksum: Correct (0x%x)\n", tcp_hdr->checksum);
    else 
        printf("\t\tChecksum: Incorrect (0x%x)\n", tcp_hdr->checksum);


}

void tcp(uint8_t *pkt_data) {
    TCP_HDR *tcp_hdr = malloc(sizeof(TCP_HDR));
    tcp_hdr->tcp_mem_addr = pkt_data;

    memcpy(&(tcp_hdr->src_port), pkt_data, 2);
    pkt_data+=2;
    memcpy(&(tcp_hdr->dest_port), pkt_data, 2);
    pkt_data+=2;
    memcpy(&(tcp_hdr->seq_num), pkt_data, 4);
    pkt_data+=4;
    memcpy(&(tcp_hdr->ack_num), pkt_data, 4);
    pkt_data+=4;
    memcpy(&(tcp_hdr->data_offset), pkt_data, 1);
    pkt_data+=1;
    memcpy(&(tcp_hdr->flag), pkt_data, 2);
    pkt_data+=2;
    memcpy(&(tcp_hdr->win_size), pkt_data, 2);
    pkt_data+=2;
    memcpy(&(tcp_hdr->checksum), pkt_data, 2);
    

    print_tcp(tcp_hdr);

    free(tcp_hdr);
}

void icmp(uint8_t *pkt_data) {
    printf("\t\tType: ");

    switch(pkt_data[0]) {
        case REQUEST: printf("Request\n");
            break;
        case REPLY: printf("Reply\n");
            break;
        default:
            break;
    }
}

void print_port(uint16_t port, uint8_t protocol) {
    switch(port) {
        case DNS: printf("DNS\n");
            break;
        case HTTP: printf("HTTP\n");
            break;
        case TELNET: printf("TELNET\n");
            break;
        case FTP: (protocol != TCP) ? printf("%d\n", port) : printf("FTP\n");
            break;
        case POP3: printf("POP3\n");
            break;
        case SMTP: printf("SMTP\n");
            break;
        default:
            printf("%d\n", port);   
    }
}
