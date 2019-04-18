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

