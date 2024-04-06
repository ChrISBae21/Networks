#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pcap/pcap.h>

#include <sys/socket.h>
#include <netinet/in.h>


#include <arpa/inet.h>

#include "ethernet.h"
#include "checksum.h"

#define UDP 17
#define ICMP 1
#define TCP 6


typedef struct __attribute__((packed)) IP_HDR {
    uint8_t *ip_hdr;
    uint8_t ver_len;
    uint8_t dscp_ecn;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
    

} IP_HDR;

void print_ip() {

}

void ip(uint8_t *pkt_data, uint8_t *len) {
    uint8_t *ip_hdr;
    uint8_t ver_len;
    uint8_t dscp_ecn;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip, dest_ip;
    uint8_t *ip;

    
    //IP_HDR *ip_hdr = malloc(sizeof(IP_HDR));

    ip_hdr = pkt_data;
    memcpy(&ver_len, pkt_data, 1);

    *len = (ver_len & 0x0F) * 4;
    printf("\t\tIP Version: %d\n", (ver_len & 0xF0) / 16);
    printf("\t\tHeader Len (bytes): %d\n", *len);
    printf("\t\tTOS subfields:\n");

    pkt_data+=1;
    memcpy(&dscp_ecn, pkt_data, 1);

    printf("\t\t\tDiffserv bits: %d\n", (dscp_ecn & 0xF0) / 16);
    printf("\t\t\tECN bits: %d\n", (dscp_ecn & 0x0F));

    pkt_data+=7;
    memcpy(&ttl, pkt_data, 1);
    printf("\t\tTTL: %d\n", ttl);

    pkt_data+=1;
    protocol = pkt_data[0];
    printf("\t\tProtocol: ");
    switch(protocol) {
        case UDP: 
            printf("UDP\n");
            break;
        case ICMP:
            printf("ICMP\n");
            
            break;
        case TCP:
            printf("TCP\n");
            break;
        default:
            printf("Unknown\n");
            break;

    }
    pkt_data+=1;
    memcpy(&checksum, pkt_data, 2);

    checksum = ntohs(checksum);
    if(in_cksum((unsigned short*) ip_hdr, *len) == 0) {
        printf("\t\tChecksum: Correct (0x%x)\n", checksum);
    }
    else {
        printf("\t\tChecksum: Incorrect (0x%x)\n", checksum);
    }

    pkt_data+=2;

    memcpy(&src_ip, pkt_data, 4);
    memcpy(&dest_ip, pkt_data+4, 4);

    struct in_addr addr = {src_ip};
    ip = (uint8_t*) inet_ntoa(addr);
    printf("\t\tSender IP: %s\n", ip);

    addr.s_addr = dest_ip;
    ip = (uint8_t*) inet_ntoa(addr);
    printf("\t\tDest IP: %s\n", ip);
    printf("\n");
}



int main(int argc, char* argv[]) {
    pcap_t* trace_file;
    uint8_t errbuf[PCAP_ERRBUF_SIZE];
    uint32_t pkt_num = 1;
    bpf_u_int32 pkt_len = 0;
    uint16_t type;

    uint8_t ip_hdr_len;

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

        switch(ntohs(type)) {
            case IP: 
                printf("\tIP Header\n");
                ip((uint8_t*) pkt_data, &ip_hdr_len);
                

                pkt_data += ip_hdr_len;     // skip the ip header
                break;
            case ARP:
                printf("ARP header\n\n");
                
                break;
            default:
                break;
                
        }

    



                




        pkt_num++;
    }
    
    
    pcap_close(trace_file);         // closes the trace file

}