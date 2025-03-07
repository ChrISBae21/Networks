#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pcap/pcap.h>
#include <arpa/inet.h>

#include "ethernet.h"
#include "ip.h"
#include "arp.h"


void IP_ARP(uint16_t type, uint8_t *pkt_data) {
    IP_HDR *ip_hdr;

    switch(ntohs(type)) {
        case IP: 
            fprintf(stdout, "\tIP Header\n");
            ip_hdr = ip(pkt_data);
            pkt_data += ip_hdr->len;            /* skip the ip header */
            icmp_tcp_udp(ip_hdr->protocol, pkt_data, ip_hdr);
            free(ip_hdr);
            break;
        case ARP:
            fprintf(stdout, "\tARP header\n");
            arp_hdr(pkt_data);
            break;
        default:
            break;
            
    }
}

int main(int argc, char* argv[]) {
    pcap_t* trace_file;
    uint8_t errbuf[PCAP_ERRBUF_SIZE];
    uint32_t pkt_num;
    bpf_u_int32 pkt_len;
    uint16_t type;

    struct pcap_pkthdr *pkt_header;
    const uint8_t *pkt_data;

    pkt_num = 1;
    pkt_len = 0;

    trace_file = pcap_open_offline(argv[1], (char*) errbuf);     /* opens the trace file */

    if(trace_file == NULL) {
        perror("pcap_open_offline() error");
        exit(-1);
    }
    /* Grab the next packet */
    while(pcap_next_ex(trace_file, &pkt_header, &pkt_data) == 1) {
        fprintf(stdout, "\n");
        pkt_len = pkt_header->caplen;                           /* packet length */
        fprintf(stdout, "Packet number: %d  Packet Len: %d\n\n", pkt_num, pkt_len);
        type = eth_hdr((uint8_t*) pkt_data);

        pkt_data += 14;             /* skip the ethernet header */
        IP_ARP(type, (uint8_t*) pkt_data);

        pkt_num++;
    }
    pcap_close(trace_file);         /* closes the trace file */

    return 0;
}