#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>

//constants
#define SERVER_PORT 3000
#define SERVER_IP "127.0.0.1"

int serverSocketSetup();

int watchdogStuff(int clientSocket);

int receive(int sock);


int main() {
    int serverSocket = serverSocketSetup();
    printf("Waiting for incoming TCP-connections...\n");
    struct sockaddr_in clientAddress;  //
    socklen_t clientAddressLen = sizeof(clientAddress);
    memset(&clientAddress, 0, sizeof(clientAddress));
    clientAddressLen = sizeof(clientAddress);
    //Accepting a new client connection
    while (1) {
        int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &clientAddressLen);
        if (clientSocket == -1) {
            printf("accept failed with error code : %d\n", errno);
            close(serverSocket);
            return -1;
        }

        printf("A new client connection accepted\n");
        watchdogStuff(clientSocket);
        close(clientSocket);
        close(serverSocket);
        exit(-1);
    }
    close(serverSocket);
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
    return serverSocket;
}

int watchdogStuff(int clientSocket) {
    int timer = 0;
    while (timer < 10) {
        sleep(1);
        timer++;
        int bytesReceived = receive(clientSocket);
        if (timer == 10) {
            break;
        }
        if (bytesReceived > 0) {
            timer = 0;
        }
    }
    return -1;
}

int receive(int clientSocket) {
    char signal[1] = {0};
    int bytesReceived = recv(clientSocket, signal, sizeof(signal), 0);
    if (bytesReceived == -1) {
        printf("recv() failed with error code : %d\n", errno);
    }
    return bytesReceived;
}
