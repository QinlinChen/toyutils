#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>

#include "error.h"
#include "protocols.h"

#define BUFSIZE         4096
#define ICMP_DATASIZE   56

/* global */
pid_t pid;
int sockfd;
struct sockaddr *sasend;
struct sockaddr *sarecv;
socklen_t salen;

/* utils */
struct addrinfo *gethostinfo(const char *host, const char *serv,
                             int family, int socktype);
char *sockaddr_to_str(const struct sockaddr *sa, socklen_t salen);
void tv_sub(struct timeval *out, struct timeval *in);
uint16_t checksum(uint16_t *buf, int len);

typedef void (*sigfunc_t)(int);
sigfunc_t signal_intr(int signo, sigfunc_t func);

/* ping */
void sig_alrm(int signo);
void readloop(void);
void proc_icmp(char *buf, ssize_t len);
void send_icmp(int sockfd, struct sockaddr *dstaddr, socklen_t addrlen);

int main(int argc, char const *argv[])
{
    const char *host, *str;
    struct addrinfo *ai;

    /* Check arguments. */
    if (argc != 2)
        app_errq("Usage: %s host", argv[0]);
    host = argv[1];

    pid = getpid() & 0xffff; /* Used for ICMP ID field.  */

    /* Send ICMP request per second */
    if (signal_intr(SIGALRM, sig_alrm) == SIG_ERR)
        unix_errq("signal error");

    /* Get IPv4 host address infomation. */
    ai = gethostinfo(host, NULL, AF_INET, 0);

    /* Print PING head. */
    str = sockaddr_to_str(ai->ai_addr, ai->ai_addrlen);
    printf("PING %s (%s) %d(%ld) bytes of data.\n",
           ai->ai_canonname ? ai->ai_canonname : str, str, ICMP_DATASIZE,
           ICMP_DATASIZE + sizeof(struct ip) + sizeof(struct icmp));

    sasend = ai->ai_addr;
    salen = ai->ai_addrlen;
    if ((sarecv = calloc(1, salen)) == NULL)
        unix_errq("calloc error");

    readloop();

    return 0;
}

struct addrinfo *gethostinfo(const char *host, const char *serv,
                             int family, int socktype)
{
    int rc;
    struct addrinfo hints, *listp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME; /* Always return canonical name. */
    hints.ai_family = family;      /* 0, AF_INET, AF_INET6, etc. */
    hints.ai_socktype = socktype;  /* 0, SOCK_STREAM, SOCK_DGRAM, etc. */

    if ((rc = getaddrinfo(host, serv, &hints, &listp)) != 0)
        app_errq("gethostinfo error for %s, %s: %s",
                 (host == NULL) ? "(no hostname)" : host,
                 (serv == NULL) ? "(no service name)" : serv,
                 gai_strerror(rc));

    return listp; /* Return pointer to first on linked list. */
}

char *sockaddr_to_str(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;
        if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
            unix_errq("inet_ntop error");
        return str;
    }

    app_errq("TODO");
    return NULL; // TODO: Support more protocals.
}

void tv_sub(struct timeval *out, struct timeval *in)
{
    if ((out->tv_usec -= in->tv_usec) < 0) { /* out -= in */
        --out->tv_sec;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

uint16_t checksum(uint16_t *buf, int len)
{
    int nleft = len;
    uint32_t sum = 0;

    while (nleft > 1) {
        sum += *buf++;
        nleft -= 2;
    }

    /* Mop up an odd byte, if necessary. */
    if (nleft == 1)
        sum += *(uint8_t *)buf;

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

sigfunc_t signal_intr(int signo, sigfunc_t func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
    if (sigaction(signo, &act, &oact) < 0)
        return SIG_ERR;
    return oact.sa_handler;
}

void readloop(void)
{
    int size;
    char recvbuf[BUFSIZE];
    ssize_t nrecv;
    socklen_t addrlen;

    if ((sockfd = socket(sasend->sa_family, SOCK_RAW, IPPROTO_ICMP)) == -1)
        unix_errq("socket error");

    size = 60 * 1024; /* OK if setsockopt fails */
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));

    sig_alrm(SIGALRM); /* Send first packet */

    while (1) {
        addrlen = salen; /* addrlen indicates the size of sarecv that can be used. */
        if ((nrecv = recvfrom(sockfd, recvbuf, sizeof(recvbuf),
                              0, sarecv, &addrlen)) < 0) {
            if (errno == EINTR) /* Interruped by SIGALRM */
                continue;
            else
                unix_errq("recvfrom error");
        }
        proc_icmp(recvbuf, nrecv);
    }
}

void sig_alrm(int signo)
{
    send_icmp(sockfd, sasend, salen);
    alarm(1);
}

void proc_icmp(char *buf, ssize_t len)
{
    int hlen, icmplen;
    double rtt;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval tvrecv, *tvsend;

    ip = (struct ip *)buf;  /* Start of IP header. */
    hlen = ip->ip_hl << 2; /* Length of IP header. */
    if (ip->ip_p != IPPROTO_ICMP)
        return; /* Not ICMP. */

    icmp = (struct icmp *)(buf + hlen); /* Start of ICMP header. */
    icmplen = len - hlen;
    if (icmplen < sizeof(struct icmp))
        return; /* Malformed packet. */

    if (icmp->icmp_type == ICMP_ECHOREPLY) {
        if (icmp->icmp_id != pid)
            return; /* Not a response to our ECHO_REQUEST. */
        if (icmplen < sizeof(struct icmp) + sizeof(struct timeval))
            return; /* Not enough data to use. */

        /* Calculate rtt. */
        if (gettimeofday(&tvrecv, NULL) == -1)
            unix_errq("gettimeofday error");
        tvsend = (struct timeval *)icmp->icmp_data;
        tv_sub(&tvrecv, tvsend);
        rtt = tvrecv.tv_sec * 1000.0 + tvrecv.tv_usec / 1000.0;

        printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
               icmplen, sockaddr_to_str(sarecv, salen),
               icmp->icmp_seq, ip->ip_ttl, rtt);
    }
}

void send_icmp(int sockfd, struct sockaddr *dstaddr, socklen_t addrlen)
{
    static int nsent = 0;
    char sendbuf[BUFSIZE];
    int sendlen;
    struct icmp *icmp;

    icmp = (struct icmp *)sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = pid;
    icmp->icmp_seq = ++nsent;
    memset(icmp->icmp_data, 0xa5, ICMP_DATASIZE); /* Fill with pattern. */
    if (gettimeofday((struct timeval *)icmp->icmp_data, NULL) == -1)
        unix_errq("gettimeofday error");

    sendlen = sizeof(struct icmp) + ICMP_DATASIZE; /* Checksum ICMP header and data. */
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = checksum((uint16_t *)icmp, sendlen);

    if (sendto(sockfd, sendbuf, sendlen, 0, dstaddr, addrlen) == -1)
        unix_errq("sendto error");
}