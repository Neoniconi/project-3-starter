#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h>
#include <string.h>
#include <netinet/in.h> 
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h> 

#include "log.h"
#include "dns_packet.h"

#define BUFFER_SIZE 8192
#define GRAPH_SIZE 256
#define INF 1<<30


