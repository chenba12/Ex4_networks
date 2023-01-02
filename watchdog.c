#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <stdlib.h>

//constants
#define SERVER_PORT 9999
#define SERVER_IP "127.0.0.1"

int watchdogSocketSetup();
int watchdogStuff();

int main() {
int watchdogSocket=watchdogSocketSetup();
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    while (1) {
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        //Accepting a new client connection
        int clientSocket = accept(watchdogSocket, (struct sockaddr *) &clientAddress, &clientAddressLen);
        if (clientSocket == -1) {
            printf("listen failed with error code : %d\n", errno);
            close(watchdogSocket);
            return -1;
        }
        printf("A new client connection accepted\n");

        //TODO something
        int status=watchdogStuff();
        if (status==-1) {
            printf("watchdog failed\n");
        }
        close(clientSocket);
    }
    return 0;
}

int watchdogSocketSetup() {
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
        printf("setSockopt() failed with error code : %d", errno);
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

int watchdogStuff() {
    int timer = 0;
    while (timer < 10) {
        //TODO recv messages
        recv();
        if (received) {
            timer = 0;
        } else {
            //TODO sleep 11 for tests

            send(); // to ping
            sleep(1);
            timer++;
        }
    }
    //TODO send to newPing
    send("timeout");
}