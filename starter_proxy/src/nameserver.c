#include "nameserver.h"


int start_nameserver(char *my_ip, unsigned short listen_port) {
    int max_fd, nready, listen_fd, len;
    fd_set read_set, read_ready_set;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_size;

    char recv_buffer[BUFFER_SIZE];
    char send_buffer[BUFFER_SIZE];

    if ((listen_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "start_nameserver: Failed to create socket\n");
        return -1;
    }

    serv_addr.sin_addr.s_addr = inet_addr(my_ip); 
    serv_addr.sin_port = htons(listen_port); 
    serv_addr.sin_family = AF_INET;

    // bind server address to socket descriptor 
    if(bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        fprintf(stderr, "start_nameserver: Failed to bind\n");
        return -1;
    }

    FD_ZERO(&read_set);
    FD_SET(listen_fd, &read_set);
    
    max_fd = listen_fd;
    printf("Initiating select loop\n");
    while (1) {
        read_ready_set = read_set;
        // printf("Watining to select...\n");
        nready = select(max_fd+1, &read_ready_set, NULL, NULL, NULL);

        if (nready > 0) {
            if (FD_ISSET(listen_fd, &read_ready_set)) {
                int client_fd;
                int client_idx;
                nready --;

                cli_size = sizeof(cli_addr);

                //receive the datagram 
                len = sizeof(cli_addr); 
                int n = recvfrom(listen_fd, recv_buffer, sizeof(recv_buffer), 
                        0, (struct sockaddr*)&cli_addr, &cli_size); //receive message from server
                
                memcpy(send_buffer, recv_buffer, BUFFER_SIZE);

                sendto(listen_fd, send_buffer, n, 0, 
                            (struct sockaddr*)&cli_addr, sizeof(cli_addr)); 

            }
        }

    }
    

}

int main(int argc, char *argv[]) {
    printf("Starting the nameserver...\n");
    int offset = 0;
    if(argc == 7)
    {
        offset = 1;
    }

    if(argc == 6 || argc == 7)
    {
        FILE *fp = fopen(argv[1+offset], "w+");
        if(fp==NULL)
        {
            printf("%s\n", argv[1+offset]);
            printf("Unable to create log file\n");
            exit(EXIT_FAILURE);
        }
        log_set_file(fp);

        unsigned short listen_port = atoi(argv[3+offset]);

        start_nameserver(argv[2+offset], listen_port);
    }
    else
    {
        printf("Wrong parameter format %d\n", argc);
    }
    return 0;
}