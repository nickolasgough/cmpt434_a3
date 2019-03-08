/* Nickolas Gough, nvg081, 11181823 */


#define STD_IN 0

#define PORT_MIN 30000
#define PORT_MAX 40000

#define MSG_SIZE 1000

#define BUF_SIZE 10

#define ID_MIN 0
#define ID_MAX 256

#define TIME_MIN 1

#define DEG_HALF 180
#define DEG_FULL 360

#define BOUNDS_MIN 0
#define BOUNDS_MAX 1000

#define BASE_X 500
#define BASE_Y 500


typedef struct {
    int x;
    int y;
} coords;

typedef struct {
    int id;
    char* address;
    char* port;
    coords loc;
    char* data;
} proc;


int check_port(char* port);

int tcp_socket(struct addrinfo** outInfo, char* mName, char* port);
