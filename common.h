/* Nickolas Gough, nvg081, 11181823 */


#define STD_IN 0

#define PORT_MIN 30000
#define PORT_MAX 40000

#define MSG_SIZE 1000

#define BOUND_MIN 0
#define BOUND_MAX 1000


int check_port(char* port);

int tcp_socket(struct addrinfo** outInfo, char* mName, char* port);
