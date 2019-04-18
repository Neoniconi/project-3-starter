#include "httpparser.h"
#include "customsocket.h"

#define INIT_BUF_SIZE 8192
#define EXTRA_URL_BUF 10
#define BYTE_LEN 8
#define MAX_CLIENTS FD_SETSIZE

struct client_struct
{
	int fd;
	char *recv_buf;
    char *send_buf;
    int is_f4m;
    size_t recv_buf_len;
    size_t recv_buf_size;
    size_t send_buf_len;
    size_t send_buf_size;
    size_t is_server;
    size_t sibling_idx;
    struct timeval *start;
    int tv_size;//the number of timeval
};

typedef struct client_struct client;

int start_proxying();