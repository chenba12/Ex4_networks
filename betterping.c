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
#include <sys/wait.h>

#define ICMP_HDRLEN 8
#define SERVER_PORT 3000
#define SERVER_IP "127.0.0.1"

unsigned short calculate_checksum(unsigned short *paddress, int len);

int clientTCPSocketSetup(char *destIP);

int receive(int clientSocket);

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Destination IP parameter is undecleared%d\n", errno);
        exit(1);
    }
    static int iterator = 0;
    char *args[2];
    char *destIP = argv[1];
    args[0] = "./watchdog";
    args[1] = NULL;
    //create a raw socket
    int rawSocket = -1;
    if ((rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
        fprintf(stderr, "socket() failed with error: %d", errno);
        fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
        return -1;
    }
    int ttl = 115;
    if (setsockopt(rawSocket, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        fprintf(stderr, "setsockopt() failed with error: %d", errno);
        exit(1);
    }
    int pid = fork();
    if (pid == 0) {
        printf("in child \n");
        execvp(args[0], args);
    }
    sleep(1);
    printf("main\n");
    struct sockaddr_in dest_in;
    memset(&dest_in, 0, sizeof(struct sockaddr_in));
    dest_in.sin_family = AF_INET;
    dest_in.sin_addr.s_addr = inet_addr(destIP);

    // Opening a new socket connection
    int clientTCPSocket = clientTCPSocketSetup(destIP);
    if (clientTCPSocket == -1) {
        printf("error\n");
    }

    // Connecting to the receiver's socket
    struct timeval start, end;
    double elapsed;
    struct icmp icmphdr; // ICMP-header
    struct icmp *pointerIcmp = &icmphdr;
    char data[IP_MAXPACKET] = "This is the ping.\n";
    size_t dataLen = strlen(data) + 1;

    printf("PING %s (%s) %zu data bytes \n", destIP, destIP, dataLen);
    while (1) {
        icmphdr.icmp_type = ICMP_ECHO;
        icmphdr.icmp_code = 0;
        icmphdr.icmp_id = 18;
        icmphdr.icmp_seq = 0;
        icmphdr.icmp_cksum = 0;
        char packet[IP_MAXPACKET];
        memcpy((packet), &icmphdr, ICMP_HDRLEN);
        memcpy(packet + ICMP_HDRLEN, data, dataLen);
        icmphdr.icmp_cksum = calculate_checksum((unsigned short *) (packet), ICMP_HDRLEN + dataLen);
        memcpy((packet), &icmphdr, ICMP_HDRLEN);
        // Calculate the ICMP header checksum
        gettimeofday(&start, NULL);
        // Send the packet using sendto() for sending datagrams.
        int bytes_sent = sendto(rawSocket, packet, ICMP_HDRLEN + dataLen, 0, (struct sockaddr *) &dest_in,
                                sizeof(dest_in));
        if (bytes_sent == -1) {
            fprintf(stderr, "sendto() failed with error: %d", errno);
            return -1;
        }

        char signal[1] = {1};
        int signalSend = send(clientTCPSocket, signal, sizeof(signal), 0);
        if (signalSend == -1) {
            printf("Send() failed with error code : %d\n", errno);
            close(clientTCPSocket);
            return -1;
        } else if (signalSend == 0) {
            printf("Peer has closed the TCP connection prior to send().\n");
            return -1;
        }

        // Get the ping response
        bzero(packet, IP_MAXPACKET);
        socklen_t len = sizeof(dest_in);
        ssize_t bytes_received;
        while ((bytes_received = recvfrom(rawSocket, packet, sizeof(packet), 0, (struct sockaddr *) &dest_in, &len))) {
            if (bytes_received <= 0) {
                fprintf(stderr, "recvfrom() failed with error: %d", errno);
            }
            if (bytes_received > 0) {
                struct iphdr *iphdr = (struct iphdr *) packet;
                struct icmphdr *icmphdr = (struct icmphdr *) (packet + (iphdr->ihl * 4));
                break;
            }
        }

        gettimeofday(&end, NULL);
        float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
        printf("%d bytes from %s icmp_seq=%d ttl=%d time=%.2f ms\n",
               bytes_sent, destIP, iterator, ttl, (milliseconds));
        iterator++;
        sleep(1);
        int status;
        if (waitpid(pid, &status, WNOHANG) != 0) {
            printf("Child process ended with exit status %d.\n", WEXITSTATUS(status));
            close(rawSocket);
            close(clientTCPSocket);
            break;
        }
    }
    return 0;
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

int clientTCPSocketSetup(char *destIP) {
    int senderSocket = 0;
    if ((senderSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to open a TCP connection : %d", errno);
        exit(-1);
    }
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    int binaryAddress = inet_pton(AF_INET, (const char *) destIP, &serverAddress.sin_addr);
    if (binaryAddress <= 0) {
        printf("Failed to convert from text to binary : %d", errno);
    }
    int connection = connect(senderSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (connection == -1) {
        printf("Connection error : %d\n", errno);
        close(senderSocket);
        exit(-1);
    }
    return senderSocket;
}

int receive(int clientSocket) {
    char timeoutMessage[8];
    int bytesReceived = recv(clientSocket, timeoutMessage, sizeof(timeoutMessage), 0);
    if (bytesReceived == -1) {
        printf("recv() failed with error code : %d\n", errno);
    }
    printf("this is signal %s", timeoutMessage);
    if (strcmp(timeoutMessage, "timeout") == 0) {
        printf("yay");
    }
    return bytesReceived;
}
