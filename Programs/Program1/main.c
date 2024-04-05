#include <stdint.h>
#include <stdio.h>

#include <pcap/pcap.h>

#include <netinet/ether.h>
#include <net/ethernet.h>>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct Ethernet {
    uint8_t dest[6];
    uint8_t src[6];
    uint8_t type[2];
} ethernet __attribute__((packed));


void ethernet_hdr() {
    // always has dest, src, and type

    uint8_t mac_addr[6];
    struct ether_addr addr = {mac_addr};


}

void print_ethernet() {

}



void print_mac(struct ether_addr addr) {
    uint8_t* ptr;

    ptr = (uint8_t) ether_ntoa(addr);
    printf("%s", ptr);

}

int main(int argc, char* argv[]) {
    pcap_t* trace_file;
    uint8_t errbuf[PCAP_ERRBUF_SIZE];

    struct pcap_pkthdr **pkt_header;
    const uint8_t **pkt_data;

    trace_file = pcap_open_offline(argv[1], (char) errbuf);     // opens the trace file

    pcap_next_ex(trace_file, pkt_header, pkt_data);
    
    pcap_close(trace_file);         // closes the trace file

}