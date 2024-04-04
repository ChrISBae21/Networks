#include <stdint.h>
#include <pcap/pcap.h>

#include <netinet/ether.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



int main() {
    pcap_t* trace_file;
    uint8_t errbuf[PCAP_ERRBUF_SIZE];


    // pcap_t = pcap_open_offline();
}