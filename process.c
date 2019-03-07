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

    char* hName;
    char* lName;
    char* lPort;

    int sockFd;
    struct addrinfo* sockInfo;
    int loggFd;
    struct addrinfo* loggInfo;

    int procFd;
    struct addrinfo* procInfo;
    struct sockaddr procAddr;
    socklen_t procLen;

    int pId;

    char* message;
    char* temp;

    char** buffer;
    int bCount;

    int oId, oCount;

    fd_set fds;
    struct timeval tv;
    int sVal;

    int n;
    int firstIter;
    int numP;

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
    hName = calloc(MSG_SIZE, sizeof(char));
    if (p.address == NULL || hName == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }
    if (gethostname(hName, MSG_SIZE) == -1) {
        printf("process: failed to determine the name of the machine\n");
        exit(1);
    }
    sprintf(p.address, "%s.usask.ca", hName);

    /* Establish port binding */
    sockFd = tcp_socket(&sockInfo, hName, p.port);
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

    /* Allocate necessary memory */
    message = calloc(MSG_SIZE, sizeof(char));
    buffer = calloc(BUF_SIZE, sizeof(char*));
    if (message == NULL || buffer == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }

    /* Generate the data packet */
    message[0] = (char) p.id;
    sprintf(&message[1], "%s", p.data);
    buffer[0] = message;
    bCount = 1;

    /* Allocate necessary memory */
    message = calloc(MSG_SIZE, sizeof(char));
    if (message == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }

    /* Begin simulated movement */
    firstIter = 1;
    while (1) {
        FD_ZERO(&fds);
        tv.tv_sec = TIME_MIN;
        tv.tv_usec = 0;
        sVal = select(sockFd + 1, &fds, NULL, NULL, &tv);

        if (sVal != 0) {
            /* Ignore for timeout */
        } else {
            /* Position process randomly */
            if (firstIter) {
                firstIter = 0;
                pos_randomly(&p.loc);
            } else {
                move_randomly(&p.loc, D);
            }

            /* Connect with logger */
            loggFd = tcp_socket(&loggInfo, lName, lPort);
            if (loggFd < 0) {
                printf("process: failed to create tcp socket for given logger\n");
                exit(1);
            }
            if (connect(loggFd, loggInfo->ai_addr, loggInfo->ai_addrlen) == -1) {
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
            send(loggFd, message, MSG_SIZE, 0);

            /* Respond to connection */
            FD_ZERO(&fds);
            FD_SET(sockFd, &fds);
            FD_SET(loggFd, &fds);
            tv.tv_sec = TIME_MIN;
            tv.tv_usec = 0;
            sVal = select(loggFd + 1, &fds, NULL, NULL, &tv);

            while (sVal == 0) {
                FD_ZERO(&fds);
                FD_SET(sockFd, &fds);
                FD_SET(loggFd, &fds);
                tv.tv_sec = TIME_MIN;
                tv.tv_usec = 0;
                sVal = select(loggFd + 1, &fds, NULL, NULL, &tv);
            }

            if (FD_ISSET(sockFd, &fds)) {
                /* Accept process connection */
                procLen = sizeof(procAddr);
                procFd = accept(sockFd, &procAddr, &procLen);
                if (procFd < 0) {
                    printf("logger: failed to accept incoming connection on socket\n");
                    exit(1);
                }

                /* Receive packer count */
                memset(message, 0, MSG_SIZE);
                recv(procFd, message, MSG_SIZE, 0);
                
                /* Receive each packet */
                numP = atoi(message);
                oCount = bCount;
                for (n = 0; n < numP; n += 1) {
                    memset(message, 0, MSG_SIZE);
                    recv(procFd, message, MSG_SIZE, 0);

                    /* Buffer given packet */
                    for (n = 0; n < BUF_SIZE; n += 1) {
                        temp = buffer[n];
                        if (temp == NULL) {
                            temp = calloc(MSG_SIZE, sizeof(char));
                            if (temp == NULL) {
                                printf("process: failed to allocate necessary memory\n");
                                exit(1);
                            }

                            temp[0] = message[0];
                            sprintf(&temp[1], "%s", &message[1]);
                            buffer[n] = temp;
                            bCount += 1;
                            break;
                        }
                        if (temp[0] == message[0]) {
                            break;
                        }
                    }

                    /* Request next packet */
                    memset(message, 0, MSG_SIZE);
                    sprintf(message, "next");
                    send(procFd, message, MSG_SIZE, 0);
                }

                /* Send packet count */
                memset(message, 0, MSG_SIZE);
                sprintf(message, "%d", bCount);
                send(procFd, message, MSG_SIZE, 0);

                /* Send each packet */
                for (n = 0; n < oCount; n += 1) {
                    send(procFd, buffer[n], MSG_SIZE, 0);

                    memset(message, 0, MSG_SIZE);
                    recv(procFd, message, MSG_SIZE, 0);
                }

                /* Terminate connection */
                close(procFd);
            }
            if (FD_ISSET(loggFd, &fds)) {
                /* Receive logger's diagnosis */
                memset(message, 0, MSG_SIZE);
                recv(loggFd, message, MSG_SIZE, 0);

                /* Logger or process */
                if (strcmp(message, "in range") == 0) {
                    /* Send packet count */
                    memset(message, 0, MSG_SIZE);
                    sprintf(message, "%d", bCount);
                    send(loggFd, message, MSG_SIZE, 0);

                    /* Send each packet */
                    for (n = 0; n < bCount; n += 1) {
                        send(loggFd, buffer[n], MSG_SIZE, 0);
                        free(buffer[n]);

                        buffer[n] = NULL;
                        bCount -= 1;

                        memset(message, 0, MSG_SIZE);
                        recv(loggFd, message, MSG_SIZE, 0);
                    }
                } else {
                    /* Request next process */
                    memset(message, 0, MSG_SIZE);
                    sprintf(message, "next");
                    send(loggFd, message, MSG_SIZE, 0);

                    /* Handle each process */
                    memset(message, 0, MSG_SIZE);
                    recv(loggFd, message, MSG_SIZE, 0);
                    while (strcmp(message, "end") != 0) {
                        /* Connect to process */
                        pId = (int) message[0];
                        procFd = tcp_socket(&procInfo, &message[1], &message[15]);
                        if (sockFd < 0) {
                            printf("process: failed to create tcp socket for given process\n");
                            exit(1);
                        }
                        if (connect(procFd, procInfo->ai_addr, procInfo->ai_addrlen) == -1) {
                            printf("process: failed to connect tcp socket for given process\n");
                            exit(1);
                        }

                        /* Send packet count */
                        memset(message, 0, MSG_SIZE);
                        sprintf(message, "%d", bCount);
                        send(procFd, message, MSG_SIZE, 0);

                        send(loggFd, message, MSG_SIZE, 0);

                        /* Send each packet */
                        for (n = 0; n < bCount; n += 1) {
                            send(procFd, buffer[n], MSG_SIZE, 0);

                            memset(message, 0, MSG_SIZE);
                            recv(procFd, message, MSG_SIZE, 0);

                            /* Record each transmission */
                            memset(message, 0, MSG_SIZE);
                            message[0] = (char) p.id;
                            message[1] = (char) pId;
                            message[2] = (char) buffer[n][0];
                            send(loggFd, message, MSG_SIZE, 0);

                            memset(message, 0, MSG_SIZE);
                            recv(loggFd, message, MSG_SIZE, 0);
                        }

                        /* Receive packet count */
                        memset(message, 0, MSG_SIZE);
                        recv(procFd, message, MSG_SIZE, 0);
                        numP = atoi(message);

                        send(loggFd, message, MSG_SIZE, 0);

                        /* Receive each packet */
                        for (n = 0; n < numP; n += 1) {
                            memset(message, 0, MSG_SIZE);
                            recv(procFd, message, MSG_SIZE, 0);

                            /* Buffer given packet */
                            for (n = 0; n < BUF_SIZE; n += 1) {
                                temp = buffer[n];
                                if (temp == NULL) {
                                    temp = calloc(MSG_SIZE, sizeof(char));
                                    if (temp == NULL) {
                                        printf("process: failed to allocate necessary memory\n");
                                        exit(1);
                                    }

                                    sprintf(temp, "%s", message);
                                    buffer[n] = temp;
                                    bCount += 1;
                                    break;
                                }
                                if (temp[0] == message[0]) {
                                    break;
                                }
                            }

                            /* Store original id */
                            oId = (int) message[0];

                            /* Request next packet */
                            memset(message, 0, MSG_SIZE);
                            sprintf(message, "next");
                            send(procFd, message, MSG_SIZE, 0);

                            /* Record each reception */
                            memset(message, 0, MSG_SIZE);
                            message[0] = (char) pId;
                            message[1] = (char) p.id;
                            message[2] = (char) oId;
                            send(loggFd, message, MSG_SIZE, 0);

                            memset(message, 0, MSG_SIZE);
                            recv(loggFd, message, MSG_SIZE, 0);
                        }

                        /* Terminate connection */
                        close(procFd);

                        /* Request next process */
                        memset(message, 0, MSG_SIZE);
                        sprintf(message, "next");
                        send(loggFd, message, MSG_SIZE, 0);

                        /* Receive next process */
                        memset(message, 0, MSG_SIZE);
                        recv(loggFd, message, MSG_SIZE, 0);
                    }
                }
            }

            /* Disconnect from logger */
            close(loggFd);
        }
    }
}
