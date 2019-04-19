/********************************************************
*  Author: Sannan Tariq                                *
*  Email: stariq@cs.cmu.edu                            *
*  Description: This code provides some basic          *
*               functionality for http message         *
*               parsing and extraction                 *
*  Bug Reports: Please send email with                 *
*              subject line "15441-proxy-bug"          *
*  Complaints/Opinions: Please send email with         *
*              subject line "15441-proxy-complaint".   *
*              This makes it easier to set a rule      *
*              to send such mail to trash :)           *
********************************************************/
#include "httpparser.h"

int max(int a, int b) {
    if (a < b) return b;

    return a;
}

/*
Implementation of memmem() function because C sucks!

The memmem() function finds the start of the first occurrence of the
       substring needle of length needlelen in the memory area haystack of
       length haystacklen.
*/
void *memmem(const void *haystack, size_t haystacklen,
                    const void *needle, size_t needlelen) {
    if (haystack == NULL || haystacklen == 0) {
        return NULL;
    }
    
    if (needle == NULL || needlelen == 0) {
        return NULL;
    }
    
    const char *h_point = haystack;
    
    for (;haystacklen >= needlelen; haystacklen --, h_point++) {
        if (!memcmp(h_point, needle, needlelen)) {
            return (void *)h_point;
        }
    }

    return NULL;
}

/*
 *  @REQUIRES:
 *  head: The header buffer
 *  head_len: The length of the header buffer
 *  url: The buffer to store the answer in
 *  
 *  @ENSURES: returns 0 if url not found, otherwise 1
 *
*/
int get_url(char *head, size_t head_len, char *url){
    char *string;
    char *end;
    if(head == NULL || url == NULL)
    {
        return 0;
    }

    string = memmem(head, head_len, "GET", strlen("GET"));
    if(string == NULL){
        return 0;
    }
    
    string += 4;
    size_t remaining_length = head_len - (string - head);
    end = memmem(string, remaining_length, " ", 1);

    if(end == NULL){
        return 0;
    }

    memcpy(url, string, end-string);

    return 1;
}

/*
 *  @REQUIRES:
 *  url: The url to detect
 *  
 *  @ENSURES: returns 1 if url is f4m, otherwise 0
 *
*/
int is_f4m(char *url){
    char *string;
    if(url == NULL)
    {
        return 0;
    }
    string = memmem(url, strlen(url), "f4m", strlen("f4m"));
    if(string == NULL || (string-url)+strlen("f4m") != strlen(url) )
    {
        return 0;
    }
    return 1;
}

/*
 *  @REQUIRES:
 *  url: The url to detect
 *  
 *  @ENSURES: returns 1 if url is video, otherwise 0
 *
*/
int is_video(char *url)
{
    char *string;
    if(url == NULL)
    {
        return 0;
    }
    string = memmem(url, strlen(url), "Seg", strlen("Seg"));
    if(string == NULL)
    {
        return 0;
    }
    string = memmem(url, strlen(url), "Frag", strlen("Frag"));
    if(string == NULL)
    {
        return 0;
    }
    return 1;
}

/*
 *  @REQUIRES:
 *  msg: msg url to detect
 *  msg_len: the length of msg
 *  bitrate: the bitrate to set
 * 
 *  @ENSURES: returns new pointer if success, otherwise NULL
 *
*/
char *replace_bitrate(char *msg, int msg_len, int bitrate, int *new_len)
{
    if(msg == NULL) return NULL;
    char *seg;
    char *vod;
    char bitrate_str[INIT_BUF_SIZE];
    sprintf(bitrate_str, "%d", bitrate);
    printf("replace with:%d\n", bitrate);
    char *new_msg = malloc(msg_len + strlen(bitrate_str));
    int offset = 0;
    
    seg = memmem(msg, msg_len, "Seg", strlen("Seg"));
    vod = memmem(msg, msg_len, "vod", strlen("vod"));
    vod = vod+4;

    memcpy(new_msg, msg, vod - msg);
    offset += vod - msg;
    memcpy(new_msg + offset, bitrate_str, strlen(bitrate_str) );
    offset += strlen(bitrate_str);
    memcpy(new_msg + offset, seg, msg_len - (seg-msg));
    offset += msg_len - (seg-msg);
    *new_len = offset;

    free(msg);

    return new_msg;
}

/*
 *  @REQUIRES:
 *  head: The header buffer
 *  head_len: The length of the header buffer
 *  key: The particular header you are looking for eg: Content-Length
 *  key_len: The legnth of the key
 *  val: The buffer to store the answer in
 *  
 *  @ENSURES: returns 0 if header not found, otherwise 1
 *
*/
int get_header_val(char *head, size_t head_len, char *key, size_t key_len, char *val)
{
    char *string;
    char *end;
    long int i;
    if (head == NULL || key == NULL || val == NULL)
    {
        return 0;
    }
    

    string = memmem(head, head_len, key, key_len);

    if (string == NULL) return 0;   /* if the key is not found*/
    
    size_t remaining_length = head_len - (string - head);
    string = memmem(string, remaining_length, ":", 1);

    if (string == NULL) return 0; /* if the header is missing a value */

    string += 2;

    remaining_length = remaining_length - (string - head);
    end = memmem(string, remaining_length, "\n", 1);

    if (end == NULL) return 0; /* if the header does not seem to end */
    
    for (i = 0; string[i] != '\n'; i++) /*copy the value into the buffer*/
    {
        val[i] = string[i];
    }
    val[i - 1] = '\0'; /* make sure the value is null terminated */

    return 1;
}


int get_content_length(char *header_buffer, size_t header_buffer_len) {
    char val[INIT_BUF_SIZE];
    int header_present;

    memset(val, '\0', INIT_BUF_SIZE);

    header_present = get_header_val(header_buffer, header_buffer_len, "Content-Length", strlen("Content-Length"), val);

    if (header_present == 0) {
        return 0;
    }

    return atoi(val);

}

int find_http_message_end(char* recv_buffer, size_t recv_buffer_len) {
    char *request_end;
    int content_length;

    if ((request_end = memmem(recv_buffer, recv_buffer_len, "\r\n\r\n", 4)) == NULL)
	{
		// fprintf(stdout, "find_http_message_end: did not find a complete http request\n");
		return -1;
	}

    /* Check whether the request has a body */

    content_length = get_content_length(recv_buffer, recv_buffer_len);

    if (request_end - recv_buffer + 4 + content_length <= recv_buffer_len) {
        return request_end - recv_buffer + 4 + content_length;
    }

    return -1;   
}


/*
 *  @REQUIRES:
 *  recv_buffer: the pointer to the buffer you want to read from
 *  recv_buffer_len: The length of the  data in the receive buffer
 *  recv_buffer_size: The total size of the receive buffer
 *  msg_len: the buffer to store the length of message
 *  
 *  @ENSURES: 
 *  - returns a pointer to the reaped HTTP message, null if none present
 *  - adjusts the receive buffer and values appropriately
*/
char *pop_message(char **recv_buffer, size_t *recv_buffer_len, size_t *recv_buffer_size, int *msg_len) {
    char *message_received, *new_recv_buffer;
    int message_length, new_recv_buffer_len;
    int new_buffer_size;

    message_length = find_http_message_end(*recv_buffer, *recv_buffer_len);

    if (message_length <= 0) return NULL;

    *msg_len = message_length;

    message_received = calloc(message_length + 1, sizeof(char));
    memcpy(message_received, *recv_buffer, message_length);

    new_buffer_size = max(INIT_BUF_SIZE, *recv_buffer_len - message_length);
    new_recv_buffer = calloc(new_buffer_size, sizeof(char));
    memcpy(new_recv_buffer, *recv_buffer + message_length, *recv_buffer_len - message_length);

    free(*recv_buffer);
    *recv_buffer = new_recv_buffer;
    *recv_buffer_len = *recv_buffer_len - message_length;
    *recv_buffer_size = new_buffer_size;


    return message_received;
}


/*
 *  @REQUIRES:
 *  buf: the pointer to the buffer you want to read from
 *  new_len: The new buffer size you want
 *  old_len: The current buffer size
 *  
 *  @ENSURES: 
 *  - returns a pointer to the new resized buffer
 *  - copies the data and frees the old buffer
*/
char *resize(char *buf, int new_len, int old_len)
{
	char *new_buf = calloc(sizeof(char), new_len);
	memcpy(new_buf, buf, old_len);
	free(buf);
	return new_buf;
}

