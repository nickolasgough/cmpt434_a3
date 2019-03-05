/* Nickolas Gough, nvg081, 11181823 */


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#include "common.h"


int main(int argc, char* argv[]) {
    int T;
    int N;

    char* hName;
    char* port;

    int sockFd;
    struct addrinfo* sockInfo;
    int clientFd;
    struct sockaddr clientAddr;
    socklen_t clientLen;

    char* message;
    int rLen;

    if (argc != 4) {
        printf("usage: ./logger <port> <T> <N>\n");
        exit(1);
    }

    /* Arguments and binding */
    port = argv[1];
    if (!check_port(port)) {
        printf("logger: port number must be between 30000 and 40000\n");
        exit(1);
    }
    T = atoi(argv[2]);
    N = atoi(argv[3]);
    if (T < 1 && N < 1) {
        printf("logger: T and N must be greater than zero\n");
        exit(1);
    }

    hName = calloc(MSG_SIZE, sizeof(char));
    if (hName == NULL) {
        printf("logger: failed to allocate necessary memory\n");
        exit(1);
    }

    if (gethostname(hName, MSG_SIZE) == -1) {
        printf("logger: failed to determine the name of the machine\n");
        exit(1);
    }
    sockFd = tcp_socket(&sockInfo, hName, port);
    if (sockFd < 0) {
        printf("logger: failed to create tcp socket for given host\n");
        exit(1);
    }

    if (bind(sockFd, sockInfo->ai_addr, sockInfo->ai_addrlen) == -1) {
        printf("logger: failed to bind tcp socket for given host\n");
        exit(1);
    }
    if (listen(sockFd, N) == -1) {
        printf("logger: failed to listen tcp socket for given host\n");
        exit(1);
    }

    message = calloc(MSG_SIZE, sizeof(char));
    if (message == NULL) {
        printf("logger: failed to allocate necessary memory\n");
        exit(1);
    }

    while (1) {
        clientLen = sizeof(clientAddr);
        clientFd = accept(sockFd, &clientAddr, &clientLen);
        if (clientFd < 0) {
            printf("logger: failed to accept incoming connection on socket\n");
            exit(1);
        }

        memset(message, 0, MSG_SIZE);
        rLen = recv(clientFd, message, MSG_SIZE, 0);
        while (rLen > 0) {
            printf("logger got id %d\n", message[0]);
            printf("logger got port %s\n", &message[1]);
            printf("logger got x %d\n", &message[7]);
            printf("logger got y %d\n", &message[12]);

            memset(message, 0, MSG_SIZE);
            rLen = recv(clientFd, message, MSG_SIZE, 0);
        }
    }
}
