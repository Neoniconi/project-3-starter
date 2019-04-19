/********************************************************
*  Author: Sannan Tariq                                *
*  Email: stariq@cs.cmu.edu                            *
*  Description: This code creates a simple             *
*                proxy with some HTTP parsing          *
*                and pipelining capabilities           *
*  Bug Reports: Please send email with                 *
*              subject line "15441-proxy-bug"          *
*  Complaints/Opinions: Please send email with         *
*              subject line "15441-proxy-complaint".   *
*              This makes it easier to set a rule      *
*              to send such mail to trash :)           *
********************************************************/
#include "proxy.h"

static double alpha;

/*
 *  @REQUIRES:
 *  client_fd: The fd of the client you want to add
 *  is_server: A flag to tell us whether this client is a server or a requester
 *  sibling_idx: For a server it will be its client, for a client it will be its server
 *  
 *  @ENSURES: returns a pointer to a new client struct
 *
*/
client *new_client(int client_fd, int is_server, size_t sibling_idx) {
    client *new = calloc(1, sizeof(client));
    new->fd = client_fd;
    new->recv_buf = calloc(INIT_BUF_SIZE, 1);
    new->send_buf = calloc(INIT_BUF_SIZE, 1);
    new->recv_buf_len = 0;
    new->send_buf_len = 0;
    new->is_server = is_server;
    new->is_f4m = 0;
    new->recv_buf_size = INIT_BUF_SIZE;
    new->send_buf_size = INIT_BUF_SIZE;
    new->sibling_idx = sibling_idx;
    new->tv_size = 0;
    new->bitrate_count = 0;
    new->throughput = 0;
    return new;
}

void free_client(client* c) {
    free(c->recv_buf);
    free(c->send_buf);
    free(c);
    return;
}

/*
 *  @REQUIRES:
 *  client_fd: The fd of the client you want to add
 *  clients: A pointer to the array of client structures
 *  read_set: The set we monitor for incoming data
 *  is_server: A flag to tell us whether this client is a server or a requester
 *  sibling_idx: For a server it will be its client's index, for a client it will be its server index
 *  
 *  @ENSURES: Returns the index of the added client if possible, otherwise -1
 *
*/
int add_client(int client_fd, client **clients, fd_set *read_set, int is_server, size_t sibling_idx) {
    int i;
    for (i = 0; i < MAX_CLIENTS - 1; i ++) {
        if (clients[i] == NULL) {
            clients[i] = new_client(client_fd, is_server, sibling_idx);
            FD_SET(client_fd, read_set);
            return i;
        }
    }
    return -1;
}

/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  read_set: The set we monitor for incoming data
 *  write_set: The set we monitor for outgoing data
 *  
 *  @ENSURES: Removes the client and its sibling from all our data structures
 *
*/
int remove_client(client **clients, size_t i, fd_set *read_set, fd_set *write_set) {
    
    if (clients[i] == NULL) {
        return -1;
    }
    printf("Removing client on fd: %d\n", clients[i]->fd);
    int sib_idx;
    close(clients[i]->fd);
    FD_CLR(clients[i]->fd, read_set);
    FD_CLR(clients[i]->fd, write_set);
    sib_idx = clients[i]->sibling_idx;
    if (clients[sib_idx] != NULL) {
        printf("Removing client on fd: %d\n", clients[sib_idx]->fd);
        close(clients[sib_idx]->fd);
        FD_CLR(clients[sib_idx]->fd, read_set);
        FD_CLR(clients[sib_idx]->fd, write_set);
        free_client(clients[sib_idx]);
        clients[sib_idx] = NULL;
    }
    free(clients[i]);
    clients[i] = NULL;
    return 0;
}

int find_maxfd(int listen_fd, client **clients) {
    int max_fd = listen_fd;
    int i;
    for (i = 0; i < MAX_CLIENTS - 1; i ++) {
        if (clients[i] != NULL) {
            if (max_fd < clients[i]->fd) {
                max_fd = clients[i]->fd;
            }
        }
    }
    return max_fd;
}

void sort_bit_rate(int bit_count, int* bit_rate)
{
    int i, j;
    for(i=0;i<bit_count;i++)
    {
        for(j=i;j<bit_count;j++)
        {
            if(bit_rate[j]>bit_rate[i])
            {
                int tmp = bit_rate[i];
                bit_rate[i] = bit_rate[j];
                bit_rate[j] = tmp;
            }
        }
    }

}


/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  
 *  @ENSURES: 
 *  - tries to send the data present in a clients send buffer to that client
 *  - If data is sent, returns the remaining bytes to send, otherwise -1
 *
*/
int process_client_send(client **clients, size_t i) {
    int n;
    char *new_send_buffer;
    
    n = send(clients[i]->fd, clients[i]->send_buf, clients[i]->send_buf_len, 0);

    if (n <= 0) {
        return -1;
    }

    size_t new_size = max(INIT_BUF_SIZE, clients[i]->send_buf_len - n);
    new_send_buffer = calloc(new_size, sizeof(char));
    memcpy(new_send_buffer, clients[i]->send_buf + n, clients[i]->send_buf_len - n);
    free(clients[i]->send_buf);
    clients[i]->send_buf = new_send_buffer;
    clients[i]->send_buf_len = clients[i]->send_buf_len - n;
    clients[i]->send_buf_size = new_size;

    return clients[i]->send_buf_len;
}

/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  
 *  @ENSURES: 
 *  - tries to recv data from the client and updates its internal state as appropriate
 *  - If data is received, return the number of bytes received, otherwise return 0 or -1
 *
*/
int recv_from_client(client** clients, size_t i) {
    int n;
    char buf[INIT_BUF_SIZE];
    size_t new_size;

    n = recv(clients[i]->fd, buf, INIT_BUF_SIZE, 0);

    
    if (n <= 0) {
        return n;
    }

    new_size = clients[i]->recv_buf_size;

    while (n > new_size - clients[i]->recv_buf_len) {
        new_size *= 2;
        
    }
    clients[i]->recv_buf = resize(clients[i]->recv_buf, 
        new_size, clients[i]->recv_buf_size);
    clients[i]->recv_buf_size = new_size;

    memcpy(&(clients[i]->recv_buf[clients[i]->recv_buf_len]), buf, n);
    clients[i]->recv_buf_len += n;

    return n;
}


/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  buf: The message to add to the send buffer
 *  
 *  @ENSURES: 
 *  - appends data to the client's send buffer and returns the number of bytes appended
 *
*/
int queue_message_send(client **clients, size_t i, char *buf, int n) {
    // size_t n = strlen(buf);
    size_t new_size;

    new_size = clients[i]->send_buf_size;

    while (n > new_size - clients[i]->send_buf_len) {
        new_size *= 2;
        
    }
    clients[i]->send_buf = resize(clients[i]->send_buf, 
        new_size, clients[i]->send_buf_size);
    clients[i]->send_buf_size = new_size;

    memcpy(&(clients[i]->send_buf[clients[i]->send_buf_len]), buf, n);
    clients[i]->send_buf_len += n;
    return n;
}


/*
 *  @REQUIRES:
 *  clients: A pointer to the array of client structures
 *  i: Index of the client to remove
 *  data_available: flag whether you can call recv on this client without blocking
 *  write_set: the set containing the fds to observe for being ready to write to
 *  
 *  @ENSURES: 
 *  - tries to read data from the client, then tries to reap a complete http message
 *      and finally tries to queue the message to be forwarded to its sibling
 *  - returns number of bytes queued if no errors, -1 otherwise
 *
*/
int process_client_read(client **clients, size_t i, int data_available, fd_set *write_set) {
    char *msg_rcvd;
    char url[INIT_BUF_SIZE];
    int msg_len;
    int nread;
    if (data_available == 1) {
        if ((nread = recv_from_client(clients, i)) < 0) {
            fprintf(stderr, "start_proxying: Error while receiving from client\n");
            return -1;
        }
        else if (nread == 0) {
            return -1;
        }
    }
    
    if ((msg_rcvd = pop_message(&(clients[i]->recv_buf), &(clients[i]->recv_buf_len), &clients[i]->recv_buf_size, &msg_len)) == NULL) {
        return 0;
    }

    else {
        int sibling_idx = clients[i]->sibling_idx;
        //replace the f4m request
        if(!clients[i]->is_server)//from client to server
        {
            struct timeval tv;
            gettimeofday(&tv,NULL);
            clients[i]->tv_size++;
            clients[i]->start = realloc(clients[i]->start, sizeof(struct timeval) * clients[i]->tv_size);
            clients[i]->start[clients[i]->tv_size-1] = tv;

            memset(url, 0, INIT_BUF_SIZE);
            if(get_url(msg_rcvd, msg_len, url))
            {
                printf("%s is f4m:%d\n", url, is_f4m(url));
                if(is_f4m(url))
                {
                    clients[i]->is_f4m = 1;
                    
                    //generate nolist request
                    char *no_list;
                    no_list = malloc(msg_len + EXTRA_URL_BUF);
                    char *offset = memmem(msg_rcvd, msg_len, ".f4m", strlen(".f4m"));
                    int prefix = offset - msg_rcvd;
                    memcpy(no_list, msg_rcvd, prefix);
                    memcpy(no_list+strlen(no_list), "_nolist", strlen("_nolist"));
                    memcpy(no_list+strlen(no_list), offset, msg_len-prefix);

                    //append nolist request to original request
                    msg_rcvd = realloc(msg_rcvd, 2*msg_len + EXTRA_URL_BUF);
                    memcpy(msg_rcvd+msg_len, no_list, strlen(no_list));

                    msg_len += strlen(no_list);

                    // printf("%s\n", msg_rcvd);
                }
                else if(is_video(url))
                {
                    printf("throughput:%d,bitrateCount:&d\n", clients[i]->throughput, clients[i]->bitrate_count);
                    int j;
                    for(j = 0; j < clients[i]->bitrate_count; j++)
                    {
                        printf("loop:%d\n", clients[i]->bit_rate[j]);
                        if(clients[i]->throughput > clients[i]->bit_rate[j] * 1.5 * 1000 
                            || j == clients[i]->bitrate_count-1)
                        {
                            int new_len;
                            msg_rcvd = replace_bitrate(msg_rcvd, msg_len, clients[i]->bit_rate[j], &new_len);
                            // msg_rcvd = replace_bitrate(msg_rcvd, msg_len, 500);
                            msg_len = new_len;

                            printf("bitrate:%d\n", clients[i]->bit_rate[j]);
                            break;
                        }
                    }
                    printf("end loop\n");
                    printf("%s\n", msg_rcvd);
                    clients[i]->chunkname_queue;
                    clients[i]->count_trunk;
                    
                }
            }
            
        }
        else//from server to client
        {
            if(clients[sibling_idx]->is_f4m)
            {
                char bitrate[INIT_BUF_SIZE];
                //todo: process f4m file
                // clients[sibling_idx];
                // msg_rcvd;
                // msg_len;
                char *offset_bitrate = memmem(msg_rcvd, msg_len, "bitrate", strlen("bitrate"));
                char *offset_bootstrapInfoId = memmem(msg_rcvd, msg_len, "bootstrapInfoId"
                    , strlen("bootstrapInfoId"));
                while(offset_bitrate != NULL)
                {
                    memset(bitrate, 0, INIT_BUF_SIZE);
                    int len = offset_bootstrapInfoId - offset_bitrate - strlen("bitrate") - 7;
                    memcpy(bitrate, offset_bitrate+strlen("bitrate")+2, len);
                    clients[sibling_idx]->bit_rate[clients[sibling_idx]->bitrate_count] = atoi(bitrate);
                    clients[sibling_idx]->bitrate_count++;
                    offset_bitrate = memmem(offset_bootstrapInfoId
                        , msg_len-(offset_bootstrapInfoId - msg_rcvd)
                        , "bitrate"
                        , strlen("bitrate"));
                    offset_bootstrapInfoId = memmem(offset_bootstrapInfoId + strlen("bootstrapInfoId")
                        , msg_len-(offset_bootstrapInfoId + strlen("bootstrapInfoId") - msg_rcvd)
                        , "bootstrapInfoId"
                        , strlen("bootstrapInfoId"));
                }
                sort_bit_rate(clients[sibling_idx]->bitrate_count, clients[sibling_idx]->bit_rate);
                clients[sibling_idx]->is_f4m = 0;
                return 0;
            }
            char val[INIT_BUF_SIZE];
            get_header_val(msg_rcvd, msg_len, "Content-Length", strlen("Content-Length"), val);
            int content_length = atoi(val);
            printf("content-length:%d\n", content_length);

            struct timeval tv;
            gettimeofday(&tv,NULL);

            struct timeval start = clients[sibling_idx]->start[0];
            int sec_tv = tv.tv_sec - start.tv_sec;
            int usec_tv = tv.tv_usec - start.tv_usec;
            double sec = sec_tv + ((double)usec_tv)/1000000;
            int throughput = BYTE_LEN * content_length / sec;
            int prev_throughput = clients[sibling_idx]->throughput;

            clients[sibling_idx]->throughput =  throughput * alpha + (1-alpha)*prev_throughput;

            //get server ip
            struct sockaddr_in addr;
            socklen_t addr_size = sizeof(struct sockaddr_in);
            int res = getpeername(clients[i]->fd, (struct sockaddr *)&addr, &addr_size);
            char clientip[20];
            strcpy(clientip, inet_ntoa(addr.sin_addr));


            log_info("%lf %d %d %d %s %s", 
                sec, throughput, clients[sibling_idx]->throughput, 
                , clientip, );

            printf("sec:%lf,this_throughput:%d, new_throughput:%d\n", 
                    sec, throughput, clients[sibling_idx]->throughput);

            clients[sibling_idx]->tv_size--;
            if(clients[sibling_idx]->tv_size > 0)
            {
                memcpy(clients[sibling_idx]->start, 
                    clients[sibling_idx]->start+sizeof(struct timeval), 
                    sizeof(struct timeval) * clients[sibling_idx]->tv_size);
                clients[sibling_idx]->start = realloc(clients[sibling_idx]->start, 
                    sizeof(struct timeval) * clients[sibling_idx]->tv_size);
            }
            else
            {
                free(clients[sibling_idx]->start);
            }
        }
        
        int bytes_queued = queue_message_send(clients, sibling_idx, msg_rcvd, msg_len);
        printf("bytes queued:%d\n", bytes_queued);
        FD_SET(clients[sibling_idx]->fd, write_set);
        return bytes_queued;
    }

}

int start_proxying() {
    int max_fd, nready, listen_fd;
    fd_set read_set, read_ready_set, write_set, write_ready_set;
    struct sockaddr_in cli_addr;
    socklen_t cli_size;
    client **clients;
    size_t i;

    unsigned short listen_port = 8888;
    char *server_ip = "3.0.0.1";
    unsigned short server_port = 8080;
    char *my_ip = "1.0.0.1";

    


    if ((listen_fd = open_listen_socket(listen_port)) < 0) {
        fprintf(stderr, "start_proxy: Failed to start listening\n");
        return -1;
    }


    // init_multiplexer(listen_fd, clients, read_set, write_set);

    clients = calloc(MAX_CLIENTS - 1, sizeof(client*));
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(listen_fd, &read_set);
    
    max_fd = listen_fd;
    printf("Initiating select loop\n");
    while (1) {
        read_ready_set = read_set;
        write_ready_set = write_set;
        // printf("Watining to select...\n");
        nready = select(max_fd+1, &read_ready_set, &write_ready_set, NULL, NULL);

        if (nready > 0) {
            if (FD_ISSET(listen_fd, &read_ready_set)) {
                int client_fd;
                int client_idx;
                nready --;

                cli_size = sizeof(cli_addr);

                if ((client_fd = accept(listen_fd, (struct sockaddr *) &cli_addr,
                                    &cli_size)) == -1) {
                    fprintf(stderr, "start_proxying: Failed to accept new connection");
                }

                // add the client to the client_fd list of filed descriptors
                else if ((client_idx = add_client(client_fd, clients, &read_set, 0, -1))!= -1) {
                    
                    int sibling_fd = open_socket_to_server(my_ip, server_ip, server_port);
                    int server_idx = add_client(sibling_fd, clients, &read_set, 1, client_idx);
                    clients[client_idx]->sibling_idx = server_idx;
                    printf("start_proxying: Connected to %s on FD %d\n"
                    "And its sibling %s on FD %d\n", inet_ntoa(cli_addr.sin_addr),
                        client_fd, server_ip, sibling_fd);

                }
                else
                    close(client_fd);         
            }

            for (i = 0; i < MAX_CLIENTS - 1 && nready > 0; i++) {
                if (clients[i] != NULL) {
                    int data_available = 0;
                    if (FD_ISSET(clients[i]->fd, &read_ready_set)) {
                        nready --;
                        data_available = 1;
                    }

                    int nread = process_client_read(clients, i, data_available, &write_set);

                    if (nread < 0) {
                        printf("read error: is server:%d\n", clients[i]->is_server);
                        if (remove_client(clients, i, &read_set, &write_set) < 0) {
                            fprintf(stderr, "start_proxying: Error removing client\n");
                        }
                    }

                    if (nread >= 0 && nready > 0) {
                        if (FD_ISSET(clients[i]->fd, &write_ready_set)) {
                            nready --;
                            int nsend = process_client_send(clients, i);
                            if (nsend < 0) {
                                printf("send error: is server:%d\n", clients[i]->is_server);
                                if (remove_client(clients, i, &read_set, &write_set) < 0) {
                                    fprintf(stderr, "start_proxying: Error removing client\n");
                                }
                            }                    
                            else if (nsend == 0) {
                                FD_CLR(clients[i]->fd, &write_set);
                            }
                        }
                    }
                }
                
            }
            max_fd = find_maxfd(listen_fd, clients);
        }

    }
    

}

int main(int argc, char *argv[]) {
    printf("Starting the proxy...\n");
    if(argc == 7 || argc == 8)
    {
        FILE *fp = fopen(argv[1], "w+");
        if(fp==NULL)
        {
            printf("%s\n", argv[1]);
            printf("Unable to create log file\n");
            exit(EXIT_FAILURE);
        }
        alpha = atof(argv[2]);
        log_set_file(fp);
        start_proxying();
        return 0;
    }
    else
    {
        printf("Wrong parameter format\n");
    }
    return 0;
}