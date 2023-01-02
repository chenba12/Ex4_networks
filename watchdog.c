#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

//constants
#define SERVER_PORT 3000
#define SERVER_IP "127.0.0.1"

int serverSocketSetup();

int watchdogStuff(int tcpSocket);

int receive(int sock, int *timer);

int sendMessage(int sock);


int main() {
    int serverSocket = serverSocketSetup();
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(serverSocket, &read_fds);
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    int ret = select(serverSocket, &read_fds, NULL, NULL, &timeout);
    if (ret < 0) {
        printf("hey :(");
        // An error occurred
    } else if (ret == 0) {
        printf("hey\n");
        // The timeout expired
    } else {
        printf("Waiting for incoming TCP-connections...\n");
        struct sockaddr_in clientAddress;  //
        socklen_t clientAddressLen = sizeof(clientAddress);
        while (1) {
            memset(&clientAddress, 0, sizeof(clientAddress));
            clientAddressLen = sizeof(clientAddress);
            //Accepting a new client connection
            int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &clientAddressLen);
            if (clientSocket == -1) {
                printf("accept failed with error code : %d\n", errno);
                close(serverSocket);
                return -1;
            }
            printf("A new client connection accepted\n");
            close(clientSocket);
        }
        close(serverSocket);
    }
    return 0;
}

int serverSocketSetup() {
    int serverSocket;
    //Opening a new TCP socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to open a TCP connection : %d", errno);
        return -1;
    }
    //Enabling reuse of the port
    int enableReuse = 1;
    int ret = setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) {
        printf("setSockopt() reuse failed with error code : %d", errno);
        return -1;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddress.sin_port = htons(SERVER_PORT);
    //bind() associates the socket with its local address 127.0.0.1
    int bindResult = bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
    if (bindResult == -1) {
        printf("Bind failed with error code : %d\n", errno);
        close(serverSocket);
        return -1;
    }
    //Preparing to accept new in coming requests
    int listenResult = listen(serverSocket, 10);
    if (listenResult == -1) {
        printf("Bind failed with error code : %d\n", errno);
        return -1;
    }
    int mode = fcntl(serverSocket, F_GETFL, 0);
    if (mode == -1) {
        printf("fcntl get failed with error code : %d\n", errno);
        exit(-1);
    }
    mode = O_NONBLOCK;
    if (fcntl(serverSocket, F_SETFL, mode) < 0) {
        printf("fcntl set failed with error code : %d\n", errno);
        exit(-1);
    }
    return serverSocket;
}

int watchdogStuff(int tcpSocket) {
    int timer = 0;
    while (timer < 10) {
        receive(tcpSocket, &timer);
        sleep(1);
    }
    //TODO send to newPing
    return 1;
}

int receive(int sock, int *timer) {
    char bufferReply[27];
    int bytesReceived = recv(sock, bufferReply, sizeof(bufferReply), 0);
    if (bytesReceived == -1) {
        printf("recv() failed with error code : %d\n", errno);
        sendMessage(sock);
        timer++;
    } else {
        timer = 0;
    }
    printf("recv %d\n", bytesReceived);
    printf("timer is %d\n", *(timer));
    printf("bufferReply %s\n", bufferReply);
    return 1;
}

int sendMessage(int sock) {
    char *okMessage = "OK!";
    size_t okMessageLen = strlen(okMessage);
    ssize_t bytesSent = send(sock, okMessage, okMessageLen, 0);
    if (bytesSent == -1) {
        printf("send() failed with error code : %d", errno);
    } else if (bytesSent == 0) {
        printf("peer has closed the TCP connection prior to send() :S.\n");
    } else if (bytesSent < okMessageLen) {
        printf("sent only %zu bytes from the required %zd.\n", okMessageLen, bytesSent);
    } else {
        printf("First part was successfully sent %zd.\n", bytesSent);
    }
    return 1;
}