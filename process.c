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


void pos_randomly(coords* pos) {
    pos->x = (rand() % BOUNDS_MAX) + 1;
    pos->y = (rand() % BOUNDS_MAX) + 1;
}


void move_randomly(coords* pos, int dist) {
    int axis = rand() % DIR_MAX;
    int dir = rand() % DIR_MAX;

    int* a = axis == 0 ? &pos->x : &pos->y;
    int d = dir == 0 ? 1 : -1;

    *a += d * dist;

    if (*a < BOUNDS_MIN) {
        *a = abs(*a);
    }
    if (*a > BOUNDS_MAX) {
        *a = (2 * BOUNDS_MAX) - *a;
    }
}


int main(int argc, char* argv[]) {
    proc p;
    int D;

    char hName[MSG_SIZE];
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

    fd_set fds;
    struct timeval tv;
    int sVal;

    if (argc != 7) {
        printf("usage: ./process <ID> <data> <D> <process port> <logger address> <logger port>\n");
        exit(1);
    }

    /* Arguments and validation */
    p.id = atoi(argv[1]);
    if (p.id < ID_MIN || p.id > ID_MAX) {
        printf("process: id must be between %d and %d\n", ID_MIN, ID_MAX);
        exit(1);
    }
    p.data = argv[2];
    if (strlen(p.data) > 10) {
        printf("process: data cannot be greater than ten characters\n");
        exit(1);
    }
    D = atoi(argv[3]);
    if (D <= BOUNDS_MIN || D >= BOUNDS_MAX) {
        printf("process: distance must be less than the bounds\n");
        exit(1);
    }
    p.port = argv[4];
    if (!check_port(p.port)) {
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

    /* Find the hostname */
    p.address = calloc(MSG_SIZE, sizeof(char));
    if (p.address == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }
    if (gethostname(p.address, MSG_SIZE) == -1) {
        printf("process: failed to determine the name of the machine\n");
        exit(1);
    }
    sprintf(p.adress, "%s.usask.ca", hName);

    /* Establish port binding */
    sockFd = tcp_socket(&sockInfo, p.address, p.port);
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

    message = calloc(MSG_SIZE, sizeof(char));
    if (message == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }

    /* Randomly position process */
    pos_randomly(&p.loc);

    /* Begin simulated movement */
    while (1) {
        FD_ZERO(&fds);
        FD_SET(STD_IN, &fds);
        FD_SET(sockFd, &fds);

        tv.tv_sec = TIME_MIN;
        tv.tv_usec = 0;
        sVal = select(sockFd + 1, &fds, NULL, NULL, &tv);

        if (sVal != 0) {
            /* Handle incoming connection */
        } else {
            /* Handle timeout reaction */
            /* Move process randomly */
            move_randomly(&p.loc, D);

            /* Connect with logger */
            loggerFd = tcp_socket(&loggerInfo, lName, lPort);
            if (loggerFd < 0) {
                printf("process: failed to create tcp socket for given logger\n");
                exit(1);
            }
            if (connect(loggerFd, loggerInfo->ai_addr, loggerInfo->ai_addrlen) == -1) {
                printf("process: failed to connect tcp socket for given logger\n");
                exit(1);
            }

            /* Send relevant data */
            memset(message, 0, MSG_SIZE);
            message[0] = (char) p.id;
            sprintf(&message[1], "%s", p.address);
            sprintf(&message[15], "%s", p.port);
            sprintf(&message[22], "%d", p.loc.x);
            sprintf(&message[27], "%d", p.loc.y);
            send(loggerFd, message, MSG_SIZE, 0);

            /* Determine within range */
            memset(message, 0, MSG_SIZE);
            recv(loggerFd, message, MSG_SIZE, 0);
            if (strcmp(message, "in range") == 0) {
                printf("process: in range\n");
            } else {
                printf("process: out of range\n");
            }

            /* Disconnect from logger */
            close(loggerFd);
        }
    }
}
