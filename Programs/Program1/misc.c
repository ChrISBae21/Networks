#include "misc.h"


/*
Prints formatted IP address
*/
void print_ip(uint32_t ip) {
    uint8_t *str_ip;
    struct in_addr addr = {ip};
    str_ip = (uint8_t*) inet_ntoa(addr);
    printf("%s\n", str_ip);

}

/*
Prints formatted MAC address
*/
void print_mac(uint8_t* mac_addr) {
    uint8_t* ptr;
    ptr = (uint8_t*) ether_ntoa((struct ether_addr*) mac_addr);
    printf("%s\n", ptr);

}
