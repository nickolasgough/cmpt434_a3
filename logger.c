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
#include <math.h>

#include "common.h"


int in_range(coords* pos, int dist) {
    int xSq  = pow(BASE_X - pos->x, 2);
    int ySq = pow(BASE_Y - pos->y, 2);
    int res = sqrt(xSq + ySq);
    return res <= dist;
}


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

    proc* procs;
    int pId;
    proc* cProc;

    int n;

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

    /* Allocate the message */
    message = calloc(MSG_SIZE, sizeof(char));
    if (message == NULL) {
        printf("logger: failed to allocate necessary memory\n");
        exit(1);
    }

    /* Allocate the processes */
    procs = calloc(N, sizeof(proc));
    if (procs == NULL) {
        printf("logger: failed to allocate necessary processes\n");
        exit(1);
    }
    for (n = 0; n < N; n += 1) {
        cProc = &procs[n];
        cProc->port = calloc(MSG_SIZE, sizeof(char));
        if (cProc->port == NULL) {
            printf("logger: failed to allocate necessary memory\n");
            exit(1);
        }
    }

    /* Handle incoming messages */
    while (1) {
        /* Handle new connection */
        clientLen = sizeof(clientAddr);
        clientFd = accept(sockFd, &clientAddr, &clientLen);
        if (clientFd < 0) {
            printf("logger: failed to accept incoming connection on socket\n");
            exit(1);
        }

        /* Handle connection requests */
        memset(message, 0, MSG_SIZE);
        rLen = recv(clientFd, message, MSG_SIZE, 0);

        /* Handle initial message */
        pId = (int) message[0];
        cProc = &procs[pId];
        cProc->id = pId;
        sprintf(cProc->port, "%s", &message[1]);
        cProc->loc.x = atoi(&message[7]);
        cProc->loc.y = atoi(&message[12]);
        printf("logger: connected to %d\n", cProc->id);
        printf("process located at coords (%d,%d)\n", cProc->loc.x, cProc->loc.y);

        /* Determine within range */
        memset(message, 0, MSG_SIZE);
        if (in_range(&cProc->loc, T)) {
            sprintf(message, "%s", "in range");
        } else {
            sprintf(message, "%s", "out of range");
        }
        printf("process is %s\n", message);
        send(clientFd, message, MSG_SIZE, 0);

        while (rLen > 0) {
            memset(message, 0, MSG_SIZE);
            rLen = recv(clientFd, message, MSG_SIZE, 0);
        }
    }
}
