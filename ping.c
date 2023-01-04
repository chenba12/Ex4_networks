#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define ICMP_HDRLEN 8


void rawSocketCreate(int *sock, int *ttl);

struct sockaddr_in
recvPing(struct sockaddr_in *dest_in, int sock, char *packet, socklen_t *len, ssize_t bytes_received);

unsigned short calculate_checksum(unsigned short *paddress, int len);


int main(int argc, char **argv) {
    //get the ip as a parameter
    char *ip = argv[1];
    static int iterator = 0;
    //a struct that holds the details of we are sending the ping to
    struct sockaddr_in dest_in;
    memset(&dest_in, 0, sizeof(struct sockaddr_in));
    dest_in.sin_family = AF_INET;
    dest_in.sin_addr.s_addr = inet_addr(ip);

    //create a raw socket
    int sock;
    int ttl;
    rawSocketCreate(&sock, &ttl);
    struct timeval start, end;
    struct icmp icmphdr; // ICMP-header
    struct icmp *pointerIcmp = &icmphdr;
    char data[IP_MAXPACKET] = "This is the ping.\n";
    size_t dataLen = strlen(data) + 1;
    printf("PING %s (%s) %zu data bytes \n", ip, ip, dataLen);
    while (1) {
        icmphdr.icmp_type = ICMP_ECHO;
        icmphdr.icmp_code = 0;
        icmphdr.icmp_id = 18;
        icmphdr.icmp_seq = iterator;
        icmphdr.icmp_cksum = 0;
        char packet[IP_MAXPACKET];
        memcpy((packet), &icmphdr, ICMP_HDRLEN);
        memcpy(packet + ICMP_HDRLEN, data, dataLen);

        icmphdr.icmp_cksum = calculate_checksum((unsigned short *) (packet), ICMP_HDRLEN + dataLen);
        memcpy((packet), &icmphdr, ICMP_HDRLEN);
        // Calculate the ICMP header checksum
        gettimeofday(&start, NULL);
        // Send the packet using sendto() for sending datagrams.
        int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + dataLen, 0, (struct sockaddr *) &dest_in, sizeof(dest_in));
        if (bytes_sent == -1) {
            fprintf(stderr, "sendto() failed with error: %d", errno);
            return -1;
        }
        // Get the ping response
        bzero(packet, IP_MAXPACKET);
        socklen_t len = sizeof(dest_in);
        ssize_t bytes_received = 0;
        dest_in = recvPing(&dest_in, sock, packet, &len, bytes_received);
        gettimeofday(&end, NULL);
        float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
        printf("%d bytes from %s icmp_seq=%d ttl=%d time=%.2f ms\n",
               bytes_sent, ip, icmphdr.icmp_seq, ttl, (milliseconds));
        iterator++;
        sleep(1);
    }
    // Close the raw socket descriptor.
    close(sock);
    return 0;
}

void rawSocketCreate(int *sock, int *ttl) {
    (*sock) = -1;
    (*ttl) = 64;
    if (((*sock) = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        fprintf(stderr, "socket() failed with error: %d", errno);
        fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
        exit(-1);
    }
    if (setsockopt((*sock), IPPROTO_IP, IP_TTL, ttl, sizeof(*ttl)) < 0) {
        fprintf(stderr, "setsockopt() failed with error: %d", errno);
        exit(-1);
    }
}

struct sockaddr_in
recvPing(struct sockaddr_in *dest_in, int sock, char *packet, socklen_t *len, ssize_t bytes_received) {
    while ((bytes_received = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *) dest_in, len))) {
        if (bytes_received <= 0) {
            fprintf(stderr, "recvfrom() failed with error: %d", errno);
        }
        if (bytes_received > 0) {
            struct iphdr *iphdr = (struct iphdr *) packet;
            struct icmphdr *icmphdr = (struct icmphdr *) (packet + (iphdr->ihl * 4));
            break;
        }
    }
    return (*dest_in);
}

unsigned short calculate_checksum(unsigned short *paddress, int len) {
    int nleft = len;
    int sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1) {
        *((unsigned char *) &answer) = *((unsigned char *) w);
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}
