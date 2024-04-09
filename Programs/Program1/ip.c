
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "checksum.h"
#include "ip.h"
#include "misc.h"

#define FIN 1   /*0b0001*/
#define SYN 2   /*0b0010*/
#define RST 4   /*0b0100*/
#define ACK 16  /*0b10000*/

/*--------------------------- IP PARSING ---------------------------*/
/*
Function that parses the IP Header into a IP_HDR struct
*/
IP_HDR* ip(uint8_t *pkt_data) {
    uint8_t ver_len;                /* Version and Length */
    uint16_t checksum;              /* Checksum */
    uint16_t tot_len;               /* total length */

    IP_HDR *ip_hdr = malloc(sizeof(IP_HDR));
    if(ip_hdr == NULL) {
        perror("IP Header Malloc Error\n");
        exit(-1);
    }
    ip_hdr->ip_mem_addr = pkt_data;             

    memcpy(&ver_len, pkt_data, 1);              /* grab ver and len byte */
    ip_hdr->ver = ((ver_len & 0xF0) / 16);      /* set Version */
    ip_hdr->len = (ver_len & 0x0F) * 4;         /* set length */
    pkt_data+=1;                                /* move 1 byte */

    memcpy(&(ip_hdr->dscp_ecn), pkt_data, 1);   /* grab dscp and ecn byte */
    pkt_data+=1;
    memcpy(&tot_len, pkt_data, 2);              /* get total length */
    ip_hdr->tot_len = ntohs(tot_len);
    pkt_data+=6;                                /* move 7 bytes */

    memcpy(&(ip_hdr->ttl), pkt_data, 1);        /* grab ttl byte */
    pkt_data+=1;                                /* move 1 byte */

    ip_hdr->protocol = pkt_data[0];             /* procotol field */
    pkt_data+=1;                                /* move 1 byte */

    memcpy(&checksum, pkt_data, 2);             /* grab checksum (2 bytes) */
    ip_hdr->checksum = ntohs(checksum);         /* get checksum in little endian */
    pkt_data+=2;                                /* move 2 bytes */

    memcpy(&(ip_hdr->src_ip), pkt_data, 4);     /* grab source IP */
    memcpy(&(ip_hdr->dest_ip), pkt_data+4, 4);  /* grab destination IP */
    print_ip_hdr(ip_hdr);                       /* print the IP header */
    return ip_hdr;
}

/*
Prints the content of the IP Header
*/
void print_ip_hdr(IP_HDR *ip_hdr) {
    
    fprintf(stdout, "\t\tIP Version: %d\n", ip_hdr->ver);
    fprintf(stdout, "\t\tHeader Len (bytes): %d\n", ip_hdr->len);
    fprintf(stdout, "\t\tTOS subfields:\n");
    fprintf(stdout, "\t\t   Diffserv bits: %d\n", (ip_hdr->dscp_ecn & 0xFC) / 4);
    fprintf(stdout, "\t\t   ECN bits: %d\n", (ip_hdr->dscp_ecn & 0x03));
    fprintf(stdout, "\t\tTTL: %d\n", ip_hdr->ttl);
    fprintf(stdout, "\t\tProtocol: ");
    switch(ip_hdr->protocol) {
        case UDP: fprintf(stdout, "UDP\n");
            break;
        case ICMP: fprintf(stdout, "ICMP\n");
            break;
        case TCP: fprintf(stdout, "TCP\n");
            break;
        default: fprintf(stdout, "Unknown\n");
            break;
    }

    if(in_cksum((unsigned short*) ip_hdr->ip_mem_addr, ip_hdr->len) == 0) 
        fprintf(stdout, "\t\tChecksum: Correct (0x%04x)\n", ip_hdr->checksum);
    else 
        fprintf(stdout, "\t\tChecksum: Incorrect (0x%04x)\n", ip_hdr->checksum);

    fprintf(stdout, "\t\tSender IP: ");
    print_ip(ip_hdr->src_ip);
    fprintf(stdout, "\t\tDest IP: ");
    print_ip(ip_hdr->dest_ip);
    
}


/*--------------------------- ICMP, TCP, AND UDP ---------------------------*/
/*
Function to identify the Protocol
*/
void icmp_tcp_udp(uint8_t protocol, uint8_t *pkt_data, IP_HDR *ip_hdr) {
    switch(protocol) {
        case UDP:   /* UDP */
            fprintf(stdout, "\n\tUDP Header\n");
            udp(pkt_data);
            break;
        case ICMP:  /* ICMP */
            fprintf(stdout, "\n\tICMP Header\n");
            icmp(pkt_data);
            break;
        case TCP:   /* TCP */
            fprintf(stdout, "\n\tTCP Header\n");
            tcp(pkt_data, ip_hdr);
            break;
        default:
            break;
    }
}

/*--------------------------- UDP ---------------------------*/
/*
Parses and prints the content of the UDP header
*/
void udp(uint8_t *pkt_data) {
    uint16_t src_port, dest_port;

    fprintf(stdout, "\t\tSource Port:  ");
    memcpy(&src_port, pkt_data, 2);
    print_port(ntohs(src_port), UDP);
    fprintf(stdout, "\t\tDest Port:  ");
    memcpy(&dest_port, pkt_data+2, 2);
    print_port(ntohs(dest_port), UDP);
}

/*--------------------------- TCP ---------------------------*/

/*
Parses the TCP header (pkt_data) and places values into the TCP_HDR struct
*/
void tcp(uint8_t *pkt_data, IP_HDR *ip_hdr) {
    uint8_t *pseudo_hdr;
    uint8_t len;

    /* allocate memory for TCP header */
    TCP_HDR *tcp_hdr = malloc(sizeof(TCP_HDR));
    if(tcp_hdr == NULL) {
        perror("TCP Header Malloc Error\n");
        exit(-1);
    }
    /* save reference to the "head" of TCP header for checksum */
    tcp_hdr->tcp_mem_addr = pkt_data;

    /* copies values into the struct */
    memcpy(&(tcp_hdr->src_port), pkt_data, 2);      /* Source Port */
    pkt_data+=2;
    memcpy(&(tcp_hdr->dest_port), pkt_data, 2);     /* Destination Port */
    pkt_data+=2;
    memcpy(&(tcp_hdr->seq_num), pkt_data, 4);       /* Sequence Number */
    pkt_data+=4;
    memcpy(&(tcp_hdr->ack_num), pkt_data, 4);       /* Ack Number */
    pkt_data+=4;

    /* length is the upper nibble */
    memcpy(&(len), pkt_data, 1);
    pkt_data+=1;
    /* mask upper nibble and multiply by 4 for number of bytes */
    tcp_hdr->data_offset = 4 * ((len & 0xF0) / 16);
    memcpy(&(tcp_hdr->flag), pkt_data, 1);          /* Flags */
    pkt_data+=1;
    memcpy(&(tcp_hdr->win_size), pkt_data, 2);      /* Window Size */
    pkt_data+=2;
    memcpy(&(tcp_hdr->checksum), pkt_data, 2);      /* Checksum Value */
    
    /* create pseudo-header */
    pseudo_hdr = mk_pseudo_hdr(ip_hdr->src_ip, ip_hdr->dest_ip, ip_hdr->protocol, ip_hdr->tot_len - ip_hdr->len);
    /* combined pseudo-header with TCP header */
    memcpy(pseudo_hdr+12, tcp_hdr->tcp_mem_addr, ip_hdr->tot_len - ip_hdr->len);

    print_tcp(tcp_hdr, pseudo_hdr, ip_hdr->tot_len - ip_hdr->len); /* print TCP header */

    free(tcp_hdr);
    free(pseudo_hdr);
}


/*
Prints the contents of the TCP header given TCP_HDR struct, and calculates checksum
*/
void print_tcp(TCP_HDR *tcp_hdr, uint8_t *pseudo_hdr, uint16_t len) {
    fprintf(stdout, "\t\tSource Port:  ");
    print_port(ntohs(tcp_hdr->src_port), TCP);

    fprintf(stdout, "\t\tDest Port:  ");
    print_port(ntohs(tcp_hdr->dest_port), TCP);

    fprintf(stdout, "\t\tSequence Number: %u\n", ntohl(tcp_hdr->seq_num));
    fprintf(stdout, "\t\tACK Number: %u\n", ntohl(tcp_hdr->ack_num));
    fprintf(stdout, "\t\tData Offset (bytes): %d\n", tcp_hdr->data_offset);

    print_tcp_flag(tcp_hdr->flag);
    fprintf(stdout, "\t\tWindow Size: %d\n", ntohs(tcp_hdr->win_size));

    if(in_cksum((unsigned short*) pseudo_hdr, 12+len) == 0) 
        fprintf(stdout, "\t\tChecksum: Correct (0x%04x)\n", ntohs(tcp_hdr->checksum));
    else 
        fprintf(stdout, "\t\tChecksum: Incorrect (0x%04x)\n", ntohs(tcp_hdr->checksum));
}

void print_tcp_flag(uint8_t flag) {
    
    fprintf(stdout, "\t\tSYN Flag: ");
    ((flag & SYN) != 0) ? fprintf(stdout, "Yes\n") : fprintf(stdout, "No\n");

    fprintf(stdout, "\t\tRST Flag: ");
    ((flag & RST) != 0) ? fprintf(stdout, "Yes\n") : fprintf(stdout, "No\n");

    fprintf(stdout, "\t\tFIN Flag: ");
    ((flag & FIN) != 0) ? fprintf(stdout, "Yes\n") : fprintf(stdout, "No\n");

    fprintf(stdout, "\t\tACK Flag: ");
    ((flag & ACK) != 0) ? fprintf(stdout, "Yes\n") : fprintf(stdout, "No\n");
}

/*
Creates TCP/UCP Pseudo header from Source IP, Dest IP, Protocol #, and TCP Length (bytes)
*/
uint8_t* mk_pseudo_hdr(uint32_t src, uint32_t dest, uint8_t protocol, uint16_t tcp_len) {
    uint8_t *pseudo_hdr;
    uint8_t *temp;
    uint16_t len;

    /* allocate memory for Pseudo Header + TCP Header + Payload */
    pseudo_hdr = calloc((12 + tcp_len), sizeof(uint8_t));   
    temp = pseudo_hdr;

    /* copies values into the pseudo header */
    memcpy(pseudo_hdr, &src, 4);
    pseudo_hdr += 4;
    memcpy(pseudo_hdr, &dest, 4);
    pseudo_hdr += 4;
    pseudo_hdr += 1;
    memcpy(pseudo_hdr, &protocol, 1);
    pseudo_hdr += 1;

    len = htons(tcp_len);
    memcpy(pseudo_hdr, &len, 2);

    return temp;
}

/*--------------------------- ICMP ---------------------------*/


/*
Function for ICMP Request/Reply
*/
void icmp(uint8_t *pkt_data) {
    fprintf(stdout, "\t\tType: ");

    switch(pkt_data[0]) {
        case REQUEST: fprintf(stdout, "Request\n");
            break;
        case REPLY: fprintf(stdout, "Reply\n");
            break;
        default: fprintf(stdout, "%u\n", pkt_data[0]);
            break;
    }
}



