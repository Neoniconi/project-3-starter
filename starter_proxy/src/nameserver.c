#include "nameserver.h"

/*
 *  @REQUIRES:
 *  graph: the distance between hosts
 *  n: the length of hosts
 *  start: the index of client in hosts
 *  servers: the index of servers in hosts
 * 
 *  @ENSURES: returns the index of target server
 *
*/
int calc_dijkstra(int graph[][GRAPH_SIZE], int n, int start, int* servers)
{
	int distance[GRAPH_SIZE];
	int visited[GRAPH_SIZE];
    int count,mindistance,nextnode,i,j, min_node, min;
    min_node = -1;
    min = INF;
		
	
	//init
	for(i=0;i<n;i++)
	{
		distance[i] = graph[start][i];
		visited[i] = 0;
	}
	
	distance[start] = 0;
	visited[start] = 1;
	
	for(count=1; count<n-1; count++)
	{
		mindistance = INF;
		
		//best next node
		for(i=0;i<n;i++)
        {
            if(distance[i]<mindistance && !visited[i])
			{
				mindistance = distance[i];
				nextnode = i;
			}
        }
			
			
        //check if better path for nxt node
        visited[nextnode] = 1;
        for(i=0;i<n;i++)
        {
            if(!visited[i])
                if(mindistance + graph[nextnode][i]<distance[i])
                {
                    distance[i] = mindistance + graph[nextnode][i];
                }
        }

	}
 
	//find the server with minimum distance
	for(i=0;i<n;i++)
    {
        if(servers[i] && distance[i]<min && i!=start)
        {
            min = distance[i];
            min_node = i;
        }
    }
    return min_node;
}

int ip_to_answer_format(char* ip, char* ans)
{
    int i,j,index;
    char buf[SIZE_32];
    index = 0;
    for(i=0, j=0; ip[i]!=0; i++)
    {
        if(ip[i] == '.')
        {
            memcpy(buf, ip+j, i-j);
            ans[index] = atoi(buf);
            index++;
            j = i+1;
        }
    }
    memcpy(buf, ip+j, i-j);
    ans[index] = atoi(buf);
    return 1;
}


int start_nameserver(int rr, char *my_ip, unsigned short listen_port, 
            char** servers, int servers_len, int lsa_graph[][GRAPH_SIZE], int hosts_len, char** hosts) 
{
    int max_fd, nready, listen_fd, len, i, index;
    fd_set read_set, read_ready_set;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_size;

    int round = 0;
    int send_len;

    char recv_buffer[BUFFER_SIZE];
    char send_buffer[BUFFER_SIZE];

    int index_is_servers[GRAPH_SIZE];
    for(i = 0; i<servers_len; i++)
    {
        if((index = find_host(hosts, hosts_len, servers[i])) != -1)
        {
            index_is_servers[index] = 1;
        }
    }

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

                char *domain_name = get_domain(recv_buffer, 0);
                uint16_t identifier = get_identifier(recv_buffer);
                uint16_t qdcount = get_qdcount(recv_buffer);
                char *cli_ip = inet_ntoa(cli_addr.sin_addr);
                char answer_ip[SIZE_32];
                memset(answer_ip, 0, SIZE_32);

                if(rr)
                {
                    ip_to_answer_format(servers[round], answer_ip);
                    dns_packet_t* packet = create_dns_packet(identifier, 
                        RESPONSE_MASK, AA_RESPONSE_MASK, qdcount, 1, RCODE_NO_ERROR);
                    add_dns_question(packet, domain_name, QTYPE_A, QCLASS_IP, 0);
                    add_dns_answer(packet, domain_name, ATYPE_A, ACLASS_IP, TTL_DEFAULT, SIZE_32, answer_ip, 0);
                    
                    char* buf = create_dns_packet_buf(packet);
                    send_len = get_pkt_len(packet);
                    memcpy(send_buffer, buf, send_len);
                    free(buf);

                    // send_len = strlen(servers[round]);
                    // memcpy(send_buffer, servers[round], send_len);
                    log_info("%s %s %s", 
                            cli_ip, domain_name, servers[round]);
                    round = (round+1) % servers_len;
                }
                else //Dijkstra
                {
                    int host_index;
                    if((host_index = find_host(hosts, hosts_len, cli_ip)) != -1)
                    {
                        printf("%s:%d\n", cli_ip, host_index);
                        index = calc_dijkstra(lsa_graph, hosts_len, host_index, index_is_servers);
                        printf("ip:%s\n", hosts[index]);
                        send_len = strlen(hosts[index]);
                        memcpy(send_buffer, hosts[index], send_len);
                        log_info("%s %s %s", 
                            cli_ip, domain_name, hosts[index]);


                        ip_to_answer_format(hosts[index], answer_ip);
                        dns_packet_t* packet = create_dns_packet(identifier, 
                            RESPONSE_MASK, AA_RESPONSE_MASK, qdcount, 1, RCODE_NO_ERROR);
                        add_dns_question(packet, domain_name, QTYPE_A, QCLASS_IP, 0);
                        add_dns_answer(packet, domain_name, ATYPE_A, ACLASS_IP, TTL_DEFAULT, SIZE_32, answer_ip, 0);

                        char* buf = create_dns_packet_buf(packet);
                        send_len = get_pkt_len(packet);
                        memcpy(send_buffer, buf, send_len);
                        free(buf);
                    }
                    else
                    {
                        printf("%s not found\n", cli_ip);
                        send_len = n;
                        memcpy(send_buffer, recv_buffer, BUFFER_SIZE);
                    }

                }
                

                sendto(listen_fd, send_buffer, send_len, 0, 
                            (struct sockaddr*)&cli_addr, sizeof(cli_addr)); 

            }
        }

    }
    

}

/*
 *  @REQUIRES:
 *  hosts: Existing hosts
 *  hosts_len: The number of existing hosts
 *  host: New hosts
 *  
 *  @ENSURES: returns index if exist, -1 if not
 *
*/
int find_host(char** hosts, int hosts_len, char* host)
{
    int i;
    for(i=0; i<hosts_len; i++)
    {
        if(strcmp(hosts[i], host) == 0)
        {
            return i;
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    printf("Starting the nameserver...\n");
    int offset = 0;
    int servers_count = 0;
    int host_count=0;
    char *servers[BUFFER_SIZE];
    int graph[GRAPH_SIZE][GRAPH_SIZE];
    char *hosts[BUFFER_SIZE];
    int hosts_seq[BUFFER_SIZE];
    char line[BUFFER_SIZE];

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

        FILE *servers_fp = fopen(argv[4+offset], "r+");
        if(servers_fp==NULL)
        {
            printf("%s\n", argv[4+offset]);
            printf("Unable to open servers file\n");
            exit(EXIT_FAILURE);
        }
        
        while (fgets(line, sizeof line, servers_fp) != NULL) /* read a line */
        {
            servers[servers_count] = malloc(strlen(line)+1);
            memcpy(servers[servers_count], line, strlen(line)-1);//strip \n at the end
            servers_count++;
        }
        fclose(servers_fp);
 
        FILE *lsa_fp = fopen(argv[5+offset], "r+");
        if(lsa_fp==NULL)
        {
            printf("%s\n", argv[5+offset]);
            printf("Unable to open servers file\n");
            exit(EXIT_FAILURE);
        }
        
        char buf[BUFFER_SIZE];
        int record[BUFFER_SIZE];
        memset(hosts_seq, 0, sizeof(int)*BUFFER_SIZE);
        memset(graph, 0, sizeof(int)*GRAPH_SIZE);
        while (fgets(line, sizeof line, lsa_fp) != NULL) /* read a line */
        {
            int i, j, host_index, seq, neighbor_index;
            memset(record, 0, BUFFER_SIZE*sizeof(int));

            //****handle sender****//
            for(i=0; line[i]!=' '; i++);
            memset(buf, 0, BUFFER_SIZE);
            memcpy(buf, line, i);
            if((host_index = find_host(hosts, host_count, buf)) == -1)
            {
                hosts[host_count] = malloc(strlen(buf)+1);
                memcpy(hosts[host_count], buf, strlen(buf));
                host_index = host_count;
                host_count++;
            }
            i++;
            
            //***handle sequence number****//
            for(j=i; line[i] != ' '; i++);
            memset(buf, 0, BUFFER_SIZE);
            memcpy(buf, line+j, i-j);
            seq = atoi(buf);
            if(seq >= hosts_seq[host_index])//ignore if former sequence
            {
                hosts_seq[host_index] = seq;
                i++;
                            
                //****handle neighbors****//
                for(j=i; line[i] != 0; i++)
                {
                    if(line[i] == ',')
                    {
                        memset(buf, 0, BUFFER_SIZE);
                        memcpy(buf, line+j, i-j);
                        j = i+1;
                        if((neighbor_index = find_host(hosts, host_count, buf)) == -1)
                        {
                            hosts[host_count] = malloc(strlen(buf)+1);
                            memcpy(hosts[host_count], buf, strlen(buf));
                            neighbor_index = host_count;
                            host_count++;
                        }
                        graph[host_index][neighbor_index] = 1;
                        record[neighbor_index] = 1;
                    }
                }
                memset(buf, 0, BUFFER_SIZE);
                memcpy(buf, line+j, i-j-2);
                if((neighbor_index = find_host(hosts, host_count, buf)) == -1)
                {
                    hosts[host_count] = malloc(strlen(buf)+1);
                    memcpy(hosts[host_count], buf, strlen(buf));
                    neighbor_index = host_count;
                    host_count++;
                }

                graph[host_index][neighbor_index] = 1;
                record[neighbor_index] = 1;

                for(i=0; i<host_count; i++)//set all the not-updated distance to INF
                {
                    if(record[i] == 0)
                    {
                        graph[host_index][i] = INF;
                    }
                }
            }
        }
        fclose(lsa_fp);

        int i, j;
        
        //set the uninitiated row to INF
        for(i=0;i<host_count;i++)
        {
            if(graph[i][0] == 0 && i != 0)
            {
                for(j = 0; j<host_count; j++)
                {
                    if(i!=j)
                        graph[i][j] = INF;
                }
            }
        }
        // for(i=0;i<host_count;i++)
        // {
        //     for(j=0;j<host_count;j++)
        //     {
        //         printf("%02d ", graph[i][j]%10);
        //     }
        //     printf("\n");
        // }

        start_nameserver(offset, argv[2+offset], listen_port, 
                servers, servers_count, graph, host_count, hosts);
    }
    else
    {
        printf("Wrong parameter format %d\n", argc);
    }
    return 0;
}