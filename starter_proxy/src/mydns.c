#include "mydns.h"

static int dns_socket;
/**
 * Initialize your client DNS library with the IP address and port number of
 * your DNS server.
 *
 * @param  dns_ip  The IP address of the DNS server.
 * @param  dns_port  The port number of the DNS server.
 *
 * @return 0 on success, -1 otherwise
 */
int init_mydns(const char *dns_ip, unsigned int dns_port)
{
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_addr.s_addr = inet_addr(dns_ip); 
    servaddr.sin_port = htons(dns_port); 
    servaddr.sin_family = AF_INET;

    dns_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if(connect(dns_socket, (struct sockaddr *)&servaddr, 
        sizeof(servaddr)) < 0)
    {
        return -1;
    }
    return 0;

}

/**
 * Resolve a DNS name using your custom DNS server.
 *
 * Whenever your proxy needs to open a connection to a web server, it calls
 * resolve() as follows:
 *
 * struct addrinfo *result;
 * int rc = resolve("video.cs.cmu.edu", "8080", null, &result);
 * if (rc != 0) {
 *     // handle error
 * }
 * // connect to address in result
 * free(result);
 *
 *
 * @param  node  The hostname to resolve.
 * @param  service  The desired port number as a string.
 * @param  hints  Should be null. resolve() ignores this parameter.
 * @param  res  The result. resolve() should allocate a struct addrinfo, which
 * the caller is responsible for freeing.
 *
 * @return 0 on success, -1 otherwise
 */
int resolve(const char *node, const char *service, 
            const struct addrinfo *hints, struct addrinfo **res)
{
    uint16_t identifier = rand()%MAX_16_UINT;
    uint16_t qrcount = 1;
    uint16_t ancount = 0;
    dns_packet_t* packet = create_dns_packet(identifier
                        , QUERY_MASK, AA_QUERY_MASK
                        , qrcount, ancount, RCODE_NO_ERROR);
    add_dns_question(packet, (char*)node, QTYPE_A, QCLASS_IP, 0);
    char* buf = create_dns_packet_buf(packet);
    sendto(dns_socket, buf, get_pkt_len(packet), 0, (struct sockaddr*)NULL, sizeof(struct sockaddr_in)); 

    char buffer[DNS_PACKET_SZ];
    recvfrom(dns_socket, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL);
    ancount = get_ancount(buffer);
    if(ancount == 0)
    {
        return -1;
    }
    int i;
    char* ip;

    ip = get_ip(buffer, 0);
    *res = (struct addrinfo*)malloc(sizeof(struct addrinfo));
    (*res)->ai_socktype = SOCK_DGRAM;
    (*res)->ai_protocol = IPPROTO_UDP;
    (*res)->ai_family = PF_INET;
    (*res)->ai_addr = malloc(sizeof(struct sockaddr));
    (*res)->ai_addr->sa_family = AF_INET;
    unsigned short port = htons(atoi(service));
    memcpy((*res)->ai_addr->sa_data, &port, 2);
    memcpy((*res)->ai_addr->sa_data+2, ip, 4);

    return 0;

}