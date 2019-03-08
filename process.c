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


/* Randomly position location */
void pos_randomly(coords* pos) {
    pos->x = (rand() % BOUNDS_MAX) + 1;
    pos->y = (rand() % BOUNDS_MAX) + 1;
}


/* Randomly move location */
void move_randomly(coords* pos, int dist) {
    int degs = rand() % DEG_FULL;
    double rads = degs * (M_PI / DEG_HALF);

    int x = dist * cos(rads);
    int y = dist * sin(rads);

    pos->x += x;
    pos->y += y;

    if (pos->x < BOUNDS_MIN) {
        pos->x = abs(pos->x);
    }
    if (pos->y < BOUNDS_MIN) {
        pos->y = abs(pos->y);
    }

    if (pos->x > BOUNDS_MAX) {
        pos->x = (2 * BOUNDS_MAX) - pos->x;
    }
    if (pos->y > BOUNDS_MAX) {
        pos->y = (2 * BOUNDS_MAX) - pos->y;
    }
}


/* Run the simulation */
int main(int argc, char* argv[]) {
    proc this;
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

    int pid;
    int cleared;

    char* message;
    char* temp;

    char** buffer;
    int bCount;

    int oId, oCount;

    fd_set fds;
    struct timeval tv;
    int sVal;

    int n;
    int fIter;
    int nPacks;

    /* Arguments and validation */
    if (argc != 7) {
        printf("usage: ./process <ID> <data> <D> <process port> <logger address> <logger port>\n");
        exit(1);
    }

    this.id = atoi(argv[1]);
    if (this.id < ID_MIN || this.id > ID_MAX) {
        printf("process: id must be between %d and %d\n", ID_MIN, ID_MAX);
        exit(1);
    }
    this.data = argv[2];
    if (strlen(this.data) > 10) {
        printf("process: data cannot be greater than ten characters\n");
        exit(1);
    }
    D = atoi(argv[3]);
    if (D <= BOUNDS_MIN || D >= BOUNDS_MAX) {
        printf("process: distance must be less than the bounds\n");
        exit(1);
    }
    this.port = argv[4];
    if (!check_port(this.port)) {
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
    this.address = calloc(MSG_SIZE, sizeof(char));
    hName = calloc(MSG_SIZE, sizeof(char));
    if (this.address == NULL || hName == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }
    if (gethostname(hName, MSG_SIZE) == -1) {
        printf("process: failed to determine the name of the machine\n");
        exit(1);
    }
    sprintf(this.address, "%s.usask.ca", hName);

    /* Establish port binding */
    sockFd = tcp_socket(&sockInfo, hName, this.port);
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
    message[0] = (char) this.id;
    sprintf(&message[1], "%s", this.data);
    buffer[0] = message;
    bCount = 1;

    /* Allocate necessary memory */
    message = calloc(MSG_SIZE, sizeof(char));
    if (message == NULL) {
        printf("process: failed to allocate necessary memory\n");
        exit(1);
    }

    /* Begin simulated movement */
    fIter = 1;
    while (1) {
        /* Periodically timeout */
        FD_ZERO(&fds);
        tv.tv_sec = TIME_MIN;
        tv.tv_usec = 0;
        sVal = select(sockFd + 1, &fds, NULL, NULL, &tv);

        /* Handle timeout response */
        if (sVal != 0) {
            /* Ignore for timeout */
        } else {
            /* Position process randomly */
            if (fIter) {
                fIter = 0;
                pos_randomly(&this.loc);
            } else {
                move_randomly(&this.loc, D);
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

            /* Send ready message */
            memset(message, 0, MSG_SIZE);
            sprintf(message, "send");
            send(loggFd, message, MSG_SIZE, 0);

            /* Wait until cleared */
            cleared = 0;
            while (!cleared) {
                /* Respond to connection */
                FD_ZERO(&fds);
                FD_SET(sockFd, &fds);
                FD_SET(loggFd, &fds);
                tv.tv_sec = TIME_MIN;
                tv.tv_usec = 0;
                sVal = select(loggFd + 1, &fds, NULL, NULL, &tv);

                /* Wait for input */
                while (sVal == 0) {
                    FD_ZERO(&fds);
                    FD_SET(sockFd, &fds);
                    FD_SET(loggFd, &fds);
                    tv.tv_sec = TIME_MIN;
                    tv.tv_usec = 0;
                    sVal = select(loggFd + 1, &fds, NULL, NULL, &tv);
                }

                /* Handle process input */
                if (FD_ISSET(sockFd, &fds)) {
                    /* Accept process connection */
                    procLen = sizeof(procAddr);
                    procFd = accept(sockFd, &procAddr, &procLen);
                    if (procFd < 0) {
                        printf("logger: failed to accept incoming connection on socket\n");
                        exit(1);
                    }

                    /* Receive packet count */
                    memset(message, 0, MSG_SIZE);
                    recv(procFd, message, MSG_SIZE, 0);
                    
                    /* Receive each packet */
                    nPacks = atoi(message);
                    oCount = bCount;
                    for (n = 0; n < nPacks; n += 1) {
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
                                printf("%d, %s\n", (int) temp[0], &temp[1]);
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
                    sprintf(message, "%d", oCount);
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

                /* Handle logger input */
                if (FD_ISSET(loggFd, &fds)) {
                    /* Logger cleared connection */
                    cleared = 1;

                    /* Receive logger's ready */
                    memset(message, 0, MSG_SIZE);
                    recv(loggFd, message, MSG_SIZE, 0);

                    /* Send relevant data */
                    memset(message, 0, MSG_SIZE);
                    message[0] = (char) this.id;
                    sprintf(&message[1], "%s", this.address);
                    sprintf(&message[15], "%s", this.port);
                    sprintf(&message[22], "%d", this.loc.x);
                    sprintf(&message[27], "%d", this.loc.y);
                    send(loggFd, message, MSG_SIZE, 0);

                    /* Receive logger's diagnosis */
                    memset(message, 0, MSG_SIZE);
                    recv(loggFd, message, MSG_SIZE, 0);

                    /* Determine within range */
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
                            pid = (int) message[0];
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

                            /* Count to logger */
                            send(loggFd, message, MSG_SIZE, 0);

                            /* Send each packet */
                            for (n = 0; n < bCount; n += 1) {
                                send(procFd, buffer[n], MSG_SIZE, 0);

                                memset(message, 0, MSG_SIZE);
                                recv(procFd, message, MSG_SIZE, 0);

                                /* Record each transmission */
                                memset(message, 0, MSG_SIZE);
                                message[0] = (char) this.id;
                                message[1] = (char) pid;
                                message[2] = (char) buffer[n][0];
                                send(loggFd, message, MSG_SIZE, 0);

                                memset(message, 0, MSG_SIZE);
                                recv(loggFd, message, MSG_SIZE, 0);
                            }

                            /* Receive packet count */
                            memset(message, 0, MSG_SIZE);
                            recv(procFd, message, MSG_SIZE, 0);
                            nPacks = atoi(message);

                            /* Count to logger */
                            send(loggFd, message, MSG_SIZE, 0);

                            /* Receive each packet */
                            for (n = 0; n < nPacks; n += 1) {
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

                                /* Store original id */
                                oId = (int) message[0];

                                /* Request next packet */
                                memset(message, 0, MSG_SIZE);
                                sprintf(message, "next");
                                send(procFd, message, MSG_SIZE, 0);

                                /* Record each reception */
                                memset(message, 0, MSG_SIZE);
                                message[0] = (char) pid;
                                message[1] = (char) this.id;
                                message[2] = (char) oId;
                                send(loggFd, message, MSG_SIZE, 0);

                                /* Wait for acknowledgement */
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
            }

            /* Disconnect from logger */
            close(loggFd);
        }
    }
}
