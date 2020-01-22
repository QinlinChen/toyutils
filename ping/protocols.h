#include <stdint.h>

struct ip {
    uint8_t     ip_hl   : 4;
    uint8_t     ip_v    : 4;
    uint8_t     ip_tos;
    uint16_t    ip_len;
    uint16_t    ip_id;
    uint16_t    ip_off;
    uint8_t     ip_ttl;
    uint8_t     ip_p;
    uint16_t    ip_sum;
    uint32_t    ip_src;
    uint32_t    ip_dst;
};

struct icmp {
    uint8_t     icmp_type;
    uint8_t     icmp_code;
    uint16_t    icmp_cksum;
    uint16_t    icmp_id;
    uint16_t    icmp_seq;
    char        icmp_data[];
};

#define ICMP_ECHOREPLY      0
#define ICMP_DEST_UNREACH   3
#define ICMP_ECHO           8