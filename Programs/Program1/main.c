#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pcap/pcap.h>

#include "ethernet.h"
#include "ip.h"
#include "arp.h"

// common TCP ports
#define HTTP 80
#define TELNET 23
#define FTP 21
#define POP3 110
#define SMTP 25


void ICMP_TCP_UDP(uint8_t protocol, uint8_t *pkt_data) {


    switch(protocol) {
        case UDP: 
            printf("\tUDP Header\n");
            break;
        case ICMP: 
            printf("\tICMP Header\n");
            break;
        case TCP:
            printf("\tTCP Header\n");
            break;
        default: printf("\tUnknown\n");
            break;
    }

}

void IP_ARP(uint16_t type, uint8_t *pkt_data) {

    
    switch(ntohs(type)) {
            case IP: 
                printf("\tIP Header\n");
                IP_HDR *ip_hdr = ip(pkt_data);
                pkt_data += ip_hdr->len;     // skip the ip header

                ICMP_TCP_UDP(ip_hdr->protocol, pkt_data);

                free(ip_hdr);
                break;
            case ARP:
                printf("\tARP header\n");
                arp_hdr(pkt_data);
                break;
            default:
                break;
                
        }
}

int main(int argc, char* argv[]) {
    pcap_t* trace_file;
    uint8_t errbuf[PCAP_ERRBUF_SIZE];
    uint32_t pkt_num = 1;
    bpf_u_int32 pkt_len = 0;
    uint16_t type;

    struct pcap_pkthdr *pkt_header;
    const uint8_t *pkt_data;

    trace_file = pcap_open_offline(argv[1], (char*) errbuf);     // opens the trace file

    if(trace_file == NULL) {
        printf("error");
    }


    // Grab the next packet
    while(pcap_next_ex(trace_file, &pkt_header, &pkt_data) == 1) {

        pkt_len = pkt_header->caplen;                           // packet length
        printf("Packet Number: %d  Packet Len: %d\n", pkt_num, pkt_len);
        type = eth_hdr((uint8_t*) pkt_data);

        pkt_data += 14;     // skip the ethernet header
        IP_ARP(type, (uint8_t*) pkt_data);
        
        pkt_num++;
    }
    
    
    pcap_close(trace_file);         // closes the trace file

}