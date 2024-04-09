#include "misc.h"

/*
Prints formatted IP address
*/
void print_ip(uint32_t ip) {
    uint8_t *str_ip;
    struct in_addr addr;
    addr.s_addr = ip;
    str_ip = (uint8_t*) inet_ntoa(addr);
    fprintf(stdout, "%s\n", str_ip);

}

/*
Prints formatted MAC address
*/
void print_mac(uint8_t* mac_addr) {
    uint8_t* ptr;
    ptr = (uint8_t*) ether_ntoa((struct ether_addr*) mac_addr);
    fprintf(stdout, "%s\n", ptr);

}

/*
Prints the Port given Port number and Protocol
*/
void print_port(uint16_t port, uint8_t protocol) {
    switch(port) {
        case DNS: fprintf(stdout, "DNS\n");
            break;
        case HTTP: fprintf(stdout, "HTTP\n");
            break;
        case TELNET: fprintf(stdout, "TELNET\n");
            break;
        case FTP: (protocol != TCP) ? fprintf(stdout, "%d\n", port) : fprintf(stdout, "FTP\n");
            break;
        case POP3: fprintf(stdout, "POP3\n");
            break;
        case SMTP: fprintf(stdout, "SMTP\n");
            break;
        default:
            fprintf(stdout, "%d\n", port);   
    }
}