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
    int id;
    char* data;
    int D;

    char* pName;
    char* pPort;

    char* lName;
    char* lPort;

    int sockFd;
    struct addrinfo* sockInfo;
    int clientFd;
    struct sockaddr clientAddr;
    socklen_t clientLen;
    int loggerFd;
    struct addrinfo* loggerInfo;

    char* message;
    int rLen;

    if (argc != 7) {
        printf("usage: ./process <ID> <data> <D> <process port> <logger address> <logger port>\n");
        exit(1);
    }

    /* Arguments and binding */
    id = atoi(argv[1]);
    if (id < 0) {
        printf("process: id must be greater than or equal to zero\n");
        exit(1);
    }
    data = argv[2];
    if (strlen(data) > 10) {
        printf("process: data cannot be greater than ten characters\n");
        exit(1);
    }
    D = atoi(argv[3]);
    if (D <= BOUND_MIN || D >= BOUND_MAX) {
        printf("process: distance must be less than the bounds\n");
        exit(1);
    }
    pPort = argv[4];
    if (!check_port(pPort)) {
        printf("process: port number must be between 30000 and 40000\n");
        exit(1);
    }
    lName = argv[5];
    if (lName == NULL) {
        printf("process: logger address must be specified\n");
        exit(1);
    }
    lPort = argv[6];
    if (!check_port(lPort)) {
        printf("process: port number must be between 30000 and 40000\n");
        exit(1);
    }

    /* Establish port binding */
    pName = calloc(MSG_SIZE, sizeof(char));
    if (pName == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }
    if (gethostname(pName, MSG_SIZE) == -1) {
        printf("process: failed to determine the name of the machine\n");
        exit(1);
    }
    sockFd = tcp_socket(&sockInfo, pName, pPort);
    if (sockFd < 0) {
        printf("process: failed to create tcp socket for given process\n");
        exit(1);
    }
    if (bind(sockFd, sockInfo->ai_addr, sockInfo->ai_addrlen) == -1) {
        printf("process: failed to bind tcp socket for given process\n");
        exit(1);
    }
    if (listen(sockFd, 1) == -1) {
        printf("process: failed to listen tcp socket for given process\n");
        exit(1);
    }

    /* Establish logger connection */
    loggerFd = tcp_socket(&loggerInfo, lName, lPort);
    if (loggerFd < 0) {
        printf("process: failed to create tcp socket for given logger\n");
        exit(1);
    }
    if (connect(loggerFd, loggerInfo->ai_addr, loggerInfo->ai_addrlen) == -1) {
        printf("process: failed to connect tcp socket for given logger\n");
        exit(1);
    }

    message = calloc(MSG_SIZE, sizeof(char));
    if (message == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }

    while (1) {
        sprintf(message, "hello");
        send(loggerFd, message, MSG_SIZE, 0);

        memset(message, 0, MSG_SIZE);
        recv(loggerFd, message, MSG_SIZE, 0);
        printf("process got message: %s\n", message);
    }
}
