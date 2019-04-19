#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 

#include <errno.h>

// #define _GNU_SOURCE
#include <string.h> 

#define INIT_BUF_SIZE 8192

int max(int a, int b);

int get_header_val(char *head, size_t head_len, char *key, size_t key_len, char *val);

char *pop_message(char **recv_buffer, size_t *recv_buffer_len, size_t *recv_buffer_size, int *msg_len);

char *resize(char *buf, int new_len, int old_len);

void *memmem(const void *haystack, size_t haystacklen,
                    const void *needle, size_t needlelen);

int get_url(char *head, size_t head_len, char *url);
int is_f4m(char *url);
int is_video(char *url);
char *replace_bitrate(char *msg, int msg_len, int bitrate, int *new_len);