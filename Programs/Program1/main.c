#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pcap/pcap.h>

#include <netinet/ether.h>
#include <net/ethernet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define IP 0x0800
#define ARP 0x0806


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

void ethernet(uint8_t *pkt_data) {
    // always has dest, src, and type
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;

    memcpy(dest, pkt_data, 6);
    memcpy(src, (pkt_data+6), 6);
    memcpy(&type, (pkt_data+12), 2);

    print_eth(dest, src, type);
    
}





int main(int argc, char* argv[]) {
    pcap_t* trace_file;
    uint8_t errbuf[PCAP_ERRBUF_SIZE];
    uint32_t pkt_num = 1;
    bpf_u_int32 pkt_len = 0;

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
        ethernet((uint8_t*) pkt_data);
        




        pkt_num++;
    }
    
    
    pcap_close(trace_file);         // closes the trace file

}