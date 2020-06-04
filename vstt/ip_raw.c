/*
 * This is the vstt Sourcecode - http://wendzel.de
 *
 * Copyright (c) 2006,2016 Steffen Wendzel, www.wendzel.de
 *       - All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "main.h"

extern struct conn_info	*ci;
extern int		 fd[2];

struct _ip	*ip;
struct _icmp	*icmp;
int		 sockfd;
#define PKTSIZE	 0xffff
char		 packet[PKTSIZE]; /* "always" enough */

void
icmp_pkt_init(u_int8_t icmp_code)
{
	bzero(&packet, PKTSIZE);
	
	/* IP protocol init */
	ip = (struct _ip *)packet;
	ip->ip_v = 4;
	ip->ip_tos = 0x0;
	ip->ip_id = htons(0);
	ip->ip_off = 0x0;
	ip->ip_ttl = 255;
	ip->ip_hl = 5;
	
	ip->ip_src.s_addr = ci->sa_my->sin_addr.s_addr;
	ip->ip_dst.s_addr = ci->sa_peer->sin_addr.s_addr;
	
	/* ICMP protocol init */
	ip->ip_p = IPPROTO_ICMP;
	
	icmp = (struct _icmp *)(packet + sizeof(struct _ip));
	icmp->icmp_type = 0; /* ECHO REPLY */
	
	if (!icmp_code)
		icmp->icmp_code = 0;
	else
		icmp->icmp_code = 1;
	
	/* sockfd */
	sockfd = -1;
}

void
icmp_kill()
{
	if (sockfd)
		close(sockfd);
	sockfd = -1;
}

unsigned short
ip_cksum(unsigned short *addr, int len)
{
	register int sum = 0;
	u_short answer = 0;
	register u_short *w = addr;
	register int nleft = len;
	
	while(nleft>1){
		sum += *w++;
		nleft -= 2;
	}
	
	if(nleft == 1){
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}
	
	sum = (sum>>16) + (sum&0xffff);
	sum += (sum>>16);
	answer = ~sum;
	
	return(answer);
}

/* set_id == 0 means not to set the id here */

int
send_icmp(char *buffer, int len, u_int16_t set_id, int16_t more)
{
	int on=1;
	
#ifdef IP_LEN_HORDER
	ip->ip_len = (sizeof(struct _ip)+sizeof(struct _icmp)+len);
#else
	ip->ip_len = htons(sizeof(struct _ip)+sizeof(struct _icmp)+len);
#endif
	
	memcpy(packet+sizeof(struct _ip)+sizeof(struct _icmp),
		buffer, len);

	if (sockfd == -1) { /* sock-init if not done before */
		if ((sockfd = socket(AF_INET, SOCK_RAW,
		    IPPROTO_ICMP)) < 0)
			err_kill_friends(1, "socket(SOCK_STREAM, IPPROTO_ICMP)" );
	
		if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL,
		    (char*) &on, sizeof(on)) < 0)
			err_kill_friends(1, "setsockopt(IP_HDRINCL)");
	}
		
	if (set_id)
		icmp->id = set_id;
	
	icmp->more_follow = more;
	
	/* do cksum stuff */
	ip->ip_sum = (unsigned short) ip_cksum((unsigned short *)ip,
	    sizeof(struct _ip));
	
	icmp->icmp_cksum = ip_cksum((unsigned short *)icmp,
	    sizeof(struct _icmp)+len);
	
	/* call ipsend(); */
	if (!sendto(sockfd, (char *)packet,
	    (sizeof(struct _ip)+sizeof(struct _icmp)+len), 0,
	    (struct sockaddr *)ci->sa_peer, sizeof(struct sockaddr_in)))
		err_kill_friends(1, "sendto");
	
	icmp_kill();
	return 0;
}

/* if more == 1 more pkts will follow to complete this send() */

void
wrapper_send_icmp(char *buffer, int len, int16_t more)
{
	static u_int16_t next_id = 1;
	u_int16_t ack_id;
	
	do {
		icmp_pkt_init(0);
		send_icmp(buffer, len, next_id, more);
		usleep(25000);
		if (read(fd[0], &ack_id, sizeof(u_int16_t))==-1) {
			/* only handle 'real' errors. we're working
			 * with non-blocking pipes here.
			 */
			if (errno != EAGAIN)
				err_kill_friends(1, "read(fifo)");
		}
	} while (ack_id != next_id);
	next_id++;
}

char *
recv_icmp(char *buffer, int *len)
{
	static char *payload = NULL;
	static char unused[]="abc\0";
	static int cur_len = 0;
	int pay_len;
	int hdr_len;
	/* struct _ip *iphdr; */ // NOT REQUIRED
	struct _icmp *icmphdr;
	char *ptr;
	u_int16_t icmp_cksum;
	static int next_id = 1;
	
	hdr_len = sizeof(struct _ip) + sizeof(struct _icmp);
	/*iphdr = (struct _ip *) buffer; */ // NOT REQUIRED
	icmphdr = (struct _icmp *) ((char *)
	    (buffer + sizeof(struct _ip))); /* C rocks! */
	pay_len = *len - hdr_len;
		
	if (payload == NULL) {
		payload = (char *) calloc(sizeof(char), pay_len + 1);
		if (!payload)
			err_kill_friends(1, __FILE__ "calloc");
	} else {
		payload = (char *) realloc(payload,
		    cur_len + pay_len + 1);
		if (!payload)
			err_kill_friends(1,  __FILE__ "realloc");
		bzero(payload+cur_len, pay_len + 1);
	}
	
	/* check if this is an ACK we are waiting for.
	 * if this is the case: send the ID into the pipe.
	 * else: go on, it's just a new pkt
	 */
	if (icmphdr->icmp_code == 1) {
		if (write(fd[1], &icmphdr->id, sizeof(u_int16_t)) == -1)
			perror("write(pipe)");
		return NULL;
	}
	
	/* before we save the buffer in payload, we've to check the
	 * checksum of the ICMP pkt and (if chksum is correct), return
	 * an answer to the peer.
	 */
	icmp_cksum = icmphdr->icmp_cksum; /* save the value */
	icmphdr->icmp_cksum = 0;	  /* set zero for re-calc */
	icmphdr->icmp_cksum = ip_cksum((unsigned short *)icmphdr,
	    sizeof(struct _icmp) + pay_len);
	if (icmp_cksum != icmphdr->icmp_cksum) {
		fprintf(stderr, "WRONG icmp chksum. \n");
		return NULL;
	}

	/* okay, cksum is correct, let's sent out an reply */
	icmp_pkt_init(1 /*Code 1*/); /* exchange dst+src ip */
	send_icmp(unused, strlen(unused), icmphdr->id, 0);
	
	/* check if the id READLY is new (maybe we already accepted
	 * a pkt with this ID but the peer did not receive the reply
	 * within the needed time)
	 */
	if (icmphdr->id != next_id)
		return NULL;
	
	next_id++;
	
	/* since everything was okay, we can save the pkt-payload from
	 * buffer in 'payload'
	 */
	memcpy(payload + cur_len, buffer + hdr_len, pay_len);
	cur_len+=pay_len;
	
	if (icmphdr->more_follow) {
		return NULL;
	} else {
		*len = cur_len;
		cur_len = 0;	/* reset for the next call */
		ptr = payload;	/* save addr for return */
		payload = NULL;	/* delete for the next call */
		return ptr;
	}
	putchar('\n');
}

