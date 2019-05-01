#include "httpparser.h"
#include "customsocket.h"
#include "log.h"
#include "mydns.h"
#include "dns_packet.h"

#define INIT_BUF_SIZE 8192
#define EXTRA_URL_BUF 10
#define CHUNK_NAME_BUF 50
#define IP_LENGTH 20
#define CONTENT_BUFFER_SIZE 20
#define BYTE_LEN 8
#define MAX_CLIENTS FD_SETSIZE
#define MAX_RATE_NUM 100

#define MAX_16_UINT 0xffff
#define DEFAULT_DOMAIN_NAME "video.cs.cmu.edu"
#define MAX_ANS 5

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
    int is_server;
    size_t sibling_idx;
    struct timeval start[CHUNK_NAME_BUF];
    int tv_req;//the number of timeval
    int tv_res;
    int bit_rate[MAX_RATE_NUM];
    int bitrate_count;
    int throughput;
    char chunkname_queue[CHUNK_NAME_BUF][CHUNK_NAME_BUF];
    int bitrate_queue[CHUNK_NAME_BUF];
    int count_req;
    int count_res;
};

typedef struct client_struct client;

int start_proxying(unsigned short listen_port, char* server_ip, char *my_ip);
