/********************************************************
*  Author: Jingze Shi, Yiting Wang                     *
*  Email: jingzes@cmu.edu yitingw@cmu.edu              *
*  Description: This code defines the DNS packet       *
*               header                                 *
********************************************************/

#ifdef _DNS_PACKET_H
#define _DNS_PACKET_H

#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#define SIZE_32 4
#define SIZE_16 2
#define SIZE_8 1

#define DNS_PACKET_SZ 512

#define QUERY_MASK 0x0
#define RESPONSE_MASK 0x8000
#define OPCODE_STANDARD_MASK 0x0
#define AA_QUERY_MASK 0x0
#define AA_RESPONSE_MASK 0x400
#define TC_FALSE_MASK 0x0
#define RD_FALSE_MASK 0x0
#define RA_FALSE_MASK 0x0
#define RESERVED_Z_MASK 0x0

#define NS_COUNT_DEFAULT 0x0
#define AR_COUNT_DEFAULT 0x0

#define DOMAIN_SEPERATOR "."

#define QTYPE_A 1
#define QCLASS_IP 1

#define ATYPE_A 1
#define ACLASS_IP 1

#define TTL_DEFAULT 0

/* Response code */
#define RCODE_NO_ERROR 0x0
#define RCODE_FORMAT_ERROR 0x1
#define RCODE_SERVER_FAILURE 0x2
#define RCODE_NAME_ERROR 0x3
#define RCODE_NOT_IMPLEMENTED 0x4
#define RCODE_REFUSED 0x5


#define HEADER_LEN 12
#define MAX_DOMAIN_LEN 256



typedef struct 
{
	uint16_t identifier;	//2byte
	uint16_t flags;			//2byte
	uint16_t qd_count;		//2byte
	uint16_t an_count;		//2byte
	uint16_t ns_count;		//2byte
	uint16_t ar_count;		//2byte
} dns_header_t;

typedef struct 
{
	char* q_name;		//2byte
	uint16_t q_type;		//2bype
	uint16_t q_class;		//2byte
	
}query_t;

typedef struct 
{
	char* name;
	uint16_t type;
	uint16_t class_name;
	uint16_t ttl;
	uint16_t rdlength;
	char* rdata;

}answer_t;

typedef struct 
{
	dns_header_t header;
	query_t** query_list;
	answer_t** answer_list;
	// char* data;
}dns_packet_t;


query_t* create_dns_question(char* name, uint16_t q_type, uint16_t q_class);
answer_t* create_dns_answer(char* name, uint16_t a_type, uint16_t a_class, 
				uint16_t ttl, uint16_t length, char* data);
dns_packet_t* create_dns_packet(uint16_t identifier, uint16_t qr
					, uint16_t aa, uint16_t qd_count
					, uint16_t an_count, uint16_t rcode);

void add_dns_question(dns_packet_t* packet, char* name
					, uint16_t q_type, uint16_t q_class, uint16_t index);
void add_dns_answer(dns_packet_t* packet, char* name, uint16_t a_type, uint16_t a_class, 
				uint16_t ttl,uint16_t length, char* data,  uint16_t index);

char* create_dns_packet_buf(dns_packet_t* packet);

void set_dns_headers(char* msg, uint16_t identifier, uint16_t flags, uint16_t qd_count
					, uint16_t an_count);
void set_dns_question(char* msg, query_t* query);
void set_dns_answer(char* msg, answer_t* answer);

uint16_t get_qdcount(char* msg);
uint16_t get_ancount(char* msg);
uint16_t get_qrcode(char* msg);
uint16_t get_answer_offset(char* msg);

char* get_domain(char* msg, uint16_t index);
char* get_ip(char* msg, int index);

void free_packet(dns_packet_t* packet);

// char* str_to_dnsname(char* domain);

#endif







