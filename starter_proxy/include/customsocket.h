#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <errno.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/time.h>

int open_listen_socket(unsigned short port);

int open_socket_to_server(char *my_ip, char *server_ip, unsigned short port);

