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
#include <time.h>

#include "common.h"


/* Check within range */
int in_range(coords* src, coords* dest, int dist) {
    int xSq  = pow(dest->x - src->x, 2);
    int ySq = pow(dest->y - src->y, 2);
    int res = sqrt(xSq + ySq);
    return res <= dist;
}


/* Allocate a process */
proc* alloc_proc() {
    proc* pProc = calloc(1, sizeof(proc));
    if (pProc == NULL) {
        printf("logger: failed to allocate necessary process\n");
        exit(1);
    }

    pProc->address = calloc(MSG_SIZE, sizeof(char));
    pProc->port = calloc(MSG_SIZE, sizeof(char));
    if (pProc->address == NULL || pProc->port == NULL) {
        printf("logger: failed to allocate necessary memory\n");
        exit(1);
    }

    return pProc;
}


/* Run the simulation */
int main(int argc, char* argv[]) {
    int T;
    int N;

    char* hName;
    char* port;

    int sockFd;
    struct addrinfo* sockInfo;
    int procFd;
    struct sockaddr procAddr;
    socklen_t procLen;

    char* message;
    char* temp;

    proc** pProcs;
    int pCount;
    int pid;
    proc* cProc;
    proc* nProc;

    coords bLoc;

    int n, c;
    int fIter;
    int nPacks;

    int nTrans;
    time_t sTime, eTime;
    double elTime;

    /* Arguments and validation */
    if (argc != 4) {
        printf("usage: ./logger <port> <T> <N>\n");
        exit(1);
    }

    port = argv[1];
    if (!check_port(port)) {
        printf("logger: port number must be between 30000 and 40000\n");
        exit(1);
    }
    T = atoi(argv[2]);
    if (T < BOUNDS_MIN || T > BOUNDS_MAX) {
        printf("logger: T must be between %d and %d\n", BOUNDS_MIN, BOUNDS_MAX);
        exit(1);
    }
    N = atoi(argv[3]);
    if (N <= ID_MIN || N >= ID_MAX) {
        printf("logger: N must be between %d and %d\n", ID_MIN + 1, ID_MAX - 1);
        exit(1);
    }

    /* Find the hostname */
    hName = calloc(MSG_SIZE, sizeof(char));
    if (hName == NULL) {
        printf("logger: failed to allocate necessary memory\n");
        exit(1);
    }
    if (gethostname(hName, MSG_SIZE) == -1) {
        printf("logger: failed to determine the name of the machine\n");
        exit(1);
    }

    /* Establish port binding */
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

    /* Set base location */
    bLoc.x = BASE_X;
    bLoc.y = BASE_Y;

    /* Allocate the processes */
    pProcs = calloc(N, sizeof(proc*));
    if (pProcs == NULL) {
        printf("logger: failed to allocate necessary processes\n");
        exit(1);
    }

    /* Initialize simulation stats */
    nTrans = 0;
    time(&sTime);

    /* Collect process data */
    pCount = 0;
    fIter = 1;
    while (pCount < N) {
        /* Format the output */
        if (fIter) {
            fIter = 0;
        } else {
            printf("\n-----\n\n");
        }

        /* Accept new connection */
        procLen = sizeof(procAddr);
        procFd = accept(sockFd, &procAddr, &procLen);
        if (procFd < 0) {
            printf("logger: failed to accept incoming connection on socket\n");
            exit(1);
        }

        /* Receive incoming request */
        memset(message, 0, MSG_SIZE);
        recv(procFd, message, MSG_SIZE, 0);

        /* Respond with clear */
        memset(message, 0, MSG_SIZE);
        sprintf(message, "clear");
        send(procFd, message, MSG_SIZE, 0);

        /* Receive updated position */
        memset(message, 0, MSG_SIZE);
        recv(procFd, message, MSG_SIZE, 0);

        /* Retrieve the process */
        pid = (int) message[0];
        cProc = pProcs[pid];
        if (cProc == NULL) {
            cProc = alloc_proc();
            pProcs[pid] = cProc;
        }

        /* Parse the updated information */
        cProc->id = pid;
        sprintf(cProc->address, "%s", &message[1]);
        sprintf(cProc->port, "%s", &message[15]);
        cProc->loc.x = atoi(&message[22]);
        cProc->loc.y = atoi(&message[27]);
        printf("logger: connected to process %d\n", cProc->id);
        printf("process address: %s, process port: %s\n", cProc->address, cProc->port);
        printf("process located at coords (%d, %d)\n", cProc->loc.x, cProc->loc.y);

        /* Determine within range */
        memset(message, 0, MSG_SIZE);
        if (in_range(&bLoc, &cProc->loc, T)) {
            sprintf(message, "%s", "in range");
        } else {
            sprintf(message, "%s", "out of range");
        }
        printf("process is %s of base station\n", message);
        send(procFd, message, MSG_SIZE, 0);

        /* Receive next command */
        memset(message, 0, MSG_SIZE);
        recv(procFd, message, MSG_SIZE, 0);
        if (strcmp(message, "next") != 0) {
            /* Receive each packet */
            nPacks = atoi(message);
            for (n = 0; n < nPacks; n += 1) {
                /* Receive a data packet */
                memset(message, 0, MSG_SIZE);
                recv(procFd, message, MSG_SIZE, 0);

                /* Print the reception */
                printf("received process %d's data from process %d\n", (int) message[0], cProc->id);

                /* Buffer given packet */
                for (c = 0; c < N; c += 1) {
                    cProc = pProcs[c];

                    if (cProc == NULL) {
                        continue;
                    }

                    pid = (int) message[0];
                    if (pid == cProc->id) {
                        if (cProc->data == NULL) {
                            temp = calloc(MSG_SIZE, sizeof(char));
                            if (temp == NULL) {
                                printf("logger: failed to allocate necessary memory\n");
                                exit(1);
                            }

                            sprintf(temp, "%s", &message[1]);
                            cProc->data = temp;
                            pCount += 1;
                            break;
                        }
                    }
                }

                /* Request next packet */
                memset(message, 0, MSG_SIZE);
                sprintf(message, "next");
                send(procFd, message, MSG_SIZE, 0);
            }
        } else {
            /* Share process information */
            for (n = 0; n < N; n += 1) {
                nProc = pProcs[n];
                if (nProc == NULL) {
                    continue;
                }
                if (nProc->id == cProc->id) {
                    continue;
                }

                if (in_range(&cProc->loc, &nProc->loc, T)) {
                    memset(message, 0, MSG_SIZE);
                    message[0] = (char) nProc->id;
                    sprintf(&message[1], "%s", nProc->address);
                    sprintf(&message[15], "%s", nProc->port);
                    send(procFd, message, MSG_SIZE, 0);

                    /* Receive packet count */
                    memset(message, 0, MSG_SIZE);
                    recv(procFd, message, MSG_SIZE, 0);

                    /* Receive exchanged packets */
                    c = atoi(message);
                    while (c > 0) {
                        memset(message, 0, MSG_SIZE);
                        recv(procFd, message, MSG_SIZE, 0);
                        c -= 1;

                        printf("logger: packet originating from %d exchanged between %d and %d\n",
                               (int) message[2], (int) message[0], (int) message[1]);
                        nTrans += 1;

                        memset(message, 0, MSG_SIZE);
                        sprintf(message, "ack");
                        send(procFd, message, MSG_SIZE, 0);
                    }

                    /* Receive packet count */
                    memset(message, 0, MSG_SIZE);
                    recv(procFd, message, MSG_SIZE, 0);

                    /* Receive exchanged packets */
                    c = atoi(message);
                    while (c > 0) {
                        memset(message, 0, MSG_SIZE);
                        recv(procFd, message, MSG_SIZE, 0);
                        c -= 1;

                        printf("logger: packet originating from %d exchanged between %d and %d\n",
                               (int) message[2], (int) message[0], (int) message[1]);
                        nTrans += 1;

                        memset(message, 0, MSG_SIZE);
                        sprintf(message, "ack");
                        send(procFd, message, MSG_SIZE, 0);
                    }

                    /* Wait for response */
                    memset(message, 0, MSG_SIZE);
                    recv(procFd, message, MSG_SIZE, 0);
                }
            }

            /* Signal the end */
            memset(message, 0, MSG_SIZE);
            sprintf(message, "end");
            send(procFd, message, MSG_SIZE, 0);
        }
    }

    /* Finalize simulation stats */
    time(&eTime);
    elTime = difftime(eTime, sTime);

    /* Format the output */
    printf("\n-----\n\n");

    /* Print simulation stats */
    printf("logger: epidemic simulation complete\n");
    for (n = 0; n < N; n += 1) {
        cProc = pProcs[n];
        printf("process %d contained data %s\n", cProc->id, cProc->data);
    }
    printf("processes transmitted data %d times over %fs\n", nTrans, elTime);

    return 0;
}
