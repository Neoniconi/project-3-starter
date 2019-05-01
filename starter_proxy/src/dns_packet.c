#include "dns_packet.h"

/*
 * Param: identifier - A 16 bit identifier assigned by the program that
 *              	   generates any kind of query.
 * Param: qr - A one bit field that specifies whether this message is a
 *             query (0), or a response (1).
 * Param: aa - Authoritative Answer
 * Param: qd_count - an unsigned 16 bit integer specifying the number of
 *                   entries in the question section.
 * Param: an_count - Header Length
 * Param: rcode - Packet Length
 * Param: flags - Packet Flags
 * Param: Adv_window - Advertised Window
 * Param: ext - Header Extension Length
 * Param: ext_data - Header Extension Data
 *
 * Purpose: To handle setting all of the packet header information in
 *  the current endianness for network transport.
 *
 * Return: A pointer to the buffer containing the headers with length plen.
 *
 * Comment: Review TCP headers for more information.
 *  http://telescript.denayer.wenk.be/~hcr/cn/idoceo/tcp_header.html
 *
 */

void set_dns_headers(char* msg, uint16_t identifier,uint16_t flags, uint16_t qd_count
					, uint16_t an_count)
{
	uint16_t tmp;
	int index = 0;

	tmp = htons(identifier);
	memcpy(msg, &tmp, SIZE_16);
	index += SIZE_16;
	tmp = htons(flags);
	memcpy(msg+index, &tmp, SIZE_16);
	index += SIZE_16;
	tmp = htons(qd_count);
	memcpy(msg+index, &tmp, SIZE_16);
	index += SIZE_16;
	tmp = htons(an_count);
	memcpy(msg+index, &tmp, SIZE_16);
	index += SIZE_16;
	tmp = htons(NS_COUNT_DEFAULT);
	memcpy(msg+index, &tmp, SIZE_16);
	index += SIZE_16;
	tmp = htons(AR_COUNT_DEFAULT);
	memcpy(msg+index, &tmp, SIZE_16);
}

char* str_to_dnsname(char* domain)
{
	char* lastdot = NULL;
	char* thisdot = NULL;
	thisdot = strstr(domain, DOMAIN_SEPERATOR);
	char* dnsname = (char*)calloc(strlen(domain)*2, sizeof(char));
	memset(dnsname, 0, strlen(domain)*2);
	int index = 0;
	while(thisdot!=NULL)
	{
		if(lastdot == NULL)
		{
			dnsname[index] = (thisdot-domain-0);
			memcpy(dnsname+index+1, domain+index, dnsname[index]);
			index+= (int)dnsname[index]+1;
		}
		else
		{
			thisdot = strstr(lastdot+1, DOMAIN_SEPERATOR);
			if(thisdot == NULL)
			{
				if(strlen(lastdot)>1)
				{
					dnsname[index] = strlen(lastdot) - 1;
					memcpy(dnsname+index+1, lastdot+1, dnsname[index]);
				}
				break;
			}
			dnsname[index] = (thisdot - lastdot) - 1;
			memcpy(dnsname+index+1, lastdot+1, dnsname[index]);
			index+=(int)dnsname[index]+1;
		}
		lastdot = thisdot;
	}
	dnsname[strlen(dnsname)] = 0;
	return dnsname;
}
	

query_t* create_dns_question(char* name, uint16_t q_type, uint16_t q_class)
{
	query_t* q = malloc(sizeof(query_t));
	q->q_name = str_to_dnsname(name);
	q->q_type = q_type;
	q->q_class = q_class;
	return q;

}

void set_dns_question(char* msg, query_t* query)
{
	uint16_t tmp;
	int index;
	memcpy(msg, query->q_name, strlen(query->q_name));
	index+= strlen(query->q_name);
	tmp = htons(query->q_type);
	memcpy(msg+index, &tmp, SIZE_16);
	index+=SIZE_16;
	tmp = htons(query->q_class);
	memcpy(msg+index, &tmp, SIZE_16);
}

answer_t* create_dns_answer(char* name, uint16_t a_type, uint16_t a_class, 
				uint16_t ttl, uint16_t length, char* data)
{
	answer_t* a = malloc(sizeof(answer_t));
	uint16_t rr_name = DEFAULT_RR_NAME;
	a->name = malloc(SIZE_16);
	memcpy(a->name, &rr_name, SIZE_16);
	// printf("%x\n", a->name[0]);
	a->type = a_type;
	a->class_name = a_class;
	a->ttl = ttl;
	a->rdlength = length;
	a->rdata = malloc(length);
	memcpy(a->rdata, data, length);
	return a;

}

void set_dns_answer(char* msg, answer_t* answer)
{
	uint16_t tmp;
	int index = 0;
	memcpy(&tmp, answer->name, SIZE_16);
	// printf("TMP: %x\n", tmp);
	tmp = htons(tmp);
	memcpy(msg, &tmp, SIZE_16);
	index+=strlen(answer->name);
	tmp = htons(answer->type);
	memcpy(msg+index, &tmp, SIZE_16);
	index+=SIZE_16;
	tmp = htons(answer->class_name);
	memcpy(msg+index, &tmp, SIZE_16);
	index+=SIZE_16;
	tmp = htons(answer->ttl);
	memcpy(msg+index, &tmp, SIZE_16);
	index+=SIZE_16;
	tmp = htons(answer->rdlength);
	memcpy(msg+index, &tmp, SIZE_16);
	index+=SIZE_16;
	memcpy(msg+index, answer->rdata, answer->rdlength);
}

dns_packet_t* create_dns_packet(uint16_t identifier, uint16_t qr
					, uint16_t aa, uint16_t qd_count
					, uint16_t an_count, uint16_t rcode)
{
	dns_packet_t* packet = malloc(sizeof(dns_packet_t));

	packet->header.identifier = identifier;
	packet->header.flags = qr|OPCODE_STANDARD_MASK
				|aa|TC_FALSE_MASK|RD_FALSE_MASK
				|RA_FALSE_MASK|RESERVED_Z_MASK|rcode;
	packet->header.qd_count = qd_count;
	packet->header.an_count = an_count;
	packet->header.ns_count = NS_COUNT_DEFAULT;
	packet->header.ar_count = AR_COUNT_DEFAULT;

	packet->query_list = malloc(qd_count*sizeof(query_t*));
	packet->answer_list = malloc(an_count*sizeof(answer_t*));
	return packet;
}


void add_dns_question(dns_packet_t* packet, char* name
					, uint16_t q_type, uint16_t q_class, uint16_t index)
{
	if(index < packet->header.qd_count)
		packet->query_list[index] = create_dns_question(name, q_type, q_class);
}

void add_dns_answer(dns_packet_t* packet, char* name, uint16_t a_type, uint16_t a_class, 
				uint16_t ttl,uint16_t length, char* data,  uint16_t index)
{
	if(index < packet->header.an_count)
		packet->answer_list[index] = create_dns_answer(name, a_type, a_class, ttl, length, data);
}

char* create_dns_packet_buf(dns_packet_t* packet)
{
	int i;
	int index = 0;
	int data_len = get_pkt_len(packet);
	char* msg = malloc(data_len*sizeof(char));
	set_dns_headers(msg, packet->header.identifier, packet->header.flags
		, packet->header.qd_count, packet->header.an_count);

	index += HEADER_LEN;
	for(i=0;i<packet->header.qd_count;i++)
	{
		set_dns_question(msg+index, packet->query_list[i]);
		index += strlen(packet->query_list[i]->q_name) + 2*SIZE_16;
	}
	for(i=0;i<packet->header.an_count;i++)
	{
		set_dns_answer(msg+index, packet->answer_list[i]);
		index += strlen(packet->answer_list[i]->name) + 4*SIZE_16
		+ packet->answer_list[i]->rdlength;
	}
	return msg;
}

int get_pkt_len(dns_packet_t* packet)
{
	int data_len = 0;
	int i;
	int index = 0;
	for(i=0;i<packet->header.qd_count;i++)
	{
		data_len += strlen(packet->query_list[i]->q_name) + 2*SIZE_16;
	}
	for(i=0;i<packet->header.an_count;i++)
	{
		data_len += SIZE_16 + 4*SIZE_16
		+ packet->answer_list[i]->rdlength;
	}
	data_len += HEADER_LEN;
	return data_len;

}

uint16_t get_qdcount(char* msg)
{
	int offset = 4;
	uint16_t var;
	memcpy(&var, msg+offset, SIZE_16);
	return ntohs(var);
}
uint16_t get_ancount(char* msg)
{
	int offset = 6;
	uint16_t var;
	memcpy(&var, msg+offset, SIZE_16);
	return ntohs(var);
}
uint16_t get_qrcode(char* msg)
{
	int offset = 2;
	uint16_t var;
	memcpy(&var, msg+offset, SIZE_16);
	uint16_t flag = ntohs(var);
	return (RESPONSE_MASK & flag)!=0;

}
uint16_t get_identifier(char* msg)
{
	int offset = 0;
	uint16_t var;
	memcpy(&var, msg, SIZE_16);
	return ntohs(var);
}

char* get_domain(char* msg, uint16_t index)
{
	uint16_t qdcount = get_qdcount(msg);
	int current = 0;
	int offset = HEADER_LEN;
	char* domain = malloc(MAX_DOMAIN_LEN);
	int domain_offset = 0;
	if(index<qdcount)
	{
		while(current<=index)
		{
			while(msg[offset]!=0)
			{
				if(current == index)
				{
					memcpy(domain+domain_offset, msg+offset+1, (int)msg[offset]);
					domain_offset += msg[offset];
					memcpy(domain+domain_offset, ".", 1);
					domain_offset += 1;
				}
				offset += msg[offset]+1;

			}
			current++;
			offset += 4;
		}

	}
	domain[strlen(domain)-1] = '\0';
	return domain;

}

uint16_t get_answer_offset(char* msg)
{
	uint16_t qdcount = get_qdcount(msg);
	int offset = HEADER_LEN;
	int current = 0;
	while(current < qdcount)
	{
		while(msg[offset]!=0)
		{
			offset += msg[offset]+1;

		}
		current++;
		offset += 4;
	}
	return offset;
}

char* get_ip(char* msg, int index)
{

	int offset = get_answer_offset(msg);
	int ancount = get_ancount(msg);
	char* ip = malloc(4);
	int current = 0;
	if(index<ancount)
	{
		while(current<=index)
		{
			offset += SIZE_16;
			offset += 4*SIZE_16;
			if(current == index)
			{
				memcpy(ip, msg+offset, 4);
				break;
			}
			offset += 4;
			current++;
		}


	}
	return ip;
}


// int main()
// {

// 	dns_packet_t* packet = create_dns_packet(12, RESPONSE_MASK, 0, 2, 2, 0);
// 	add_dns_question(packet, "domain.com", 1, 1, 0);	
// 	add_dns_question(packet, "domain.com", 1, 1, 1);
// 	char ip[4];
// 	ip[0] = 1;
// 	ip[1] = 2;
// 	ip[2] = 3;
// 	ip[3] = 4;
// 	add_dns_answer(packet, "domain.commmmm", 1,1,0,4,ip,0);
// 	add_dns_answer(packet, "domain.commmmm", 1,1,0,4,ip,1);

// 	char* buf = create_dns_packet_buf(packet);
// 	int qdcount = get_qdcount(buf);
// 	int ancount = get_ancount(buf);
// 	int qrcode = get_qrcode(buf);
// 	printf("Qdcount: %d ancount: %d qrcode: %d\n", qdcount, ancount, qrcode);
// 	char* domain = get_domain(buf, 1);
// 	printf("Domain: %s\n", domain);
// 	int answer_offset = get_answer_offset(buf);
// 	uint16_t identifier = get_identifier(buf);
// 	printf("identifier: %d\n", identifier);
// 	printf("%d\n", answer_offset);
// 	char* getIp = get_ip(buf, 1);
// 	printf("%d\n", getIp[2]);
// }



