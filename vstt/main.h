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

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define __FAVOR_BSD
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <string.h>
#ifdef __svr4__
  #include <strings.h>
  typedef uint8_t u_int8_t;
  typedef uint16_t u_int16_t;
  typedef uint32_t u_int32_t;
#endif

#define VSTT_VERSION		"0.5.3"
#define VSTT_PATCHLEVEL		"p0"
#define VSTT_VER		VSTT_VERSION VSTT_PATCHLEVEL

/* fifo paths */
#ifndef SEND2PEER_FIFO
   #define SEND2PEER_FIFO	"/tmp/.vstt_send2peer"
#endif
#ifndef RECVFPEER_FIFO
   #define RECVFPEER_FIFO	"/tmp/.vstt_recvfpeer"
#endif

#define S2F_PATH		"/usr/sbin/s2f"

#define max(a,b)		(a > b ? a : b)

#define SETSIZE(v)				\
	v = (ci->family == AF_INET ?		\
		sizeof(struct sockaddr_in) :	\
		sizeof(struct sockaddr_in6));

/* This can be tuned if everything works. values of 64
 * or even more (I ran some tests with 92!) should be
 * possible!
 */
#define ICMP_PAYLOAD_SIZE	60	// bytes
#define USLEEP_VALUE		75000	// micro sec

#define FFLUSH		{ fflush(stdout); fflush(stderr); }

enum protocol {
	P_icmp,
	P_pop3,
	P_none,
	P_max
};

struct conn_info {
	struct sockaddr_storage	 ss_peer;
	struct sockaddr_storage	 ss_my;
	struct sockaddr_in	*sa_peer;
	struct sockaddr_in	*sa_my;
	struct sockaddr_in6	*sa6_peer;
	struct sockaddr_in6	*sa6_my;
	int			 sockfd_my;
	int			 sockfd_peer;
	int			 family;
};

/* abk. */
#define _ip _iphdr
#define _icmp _icmphdr

struct _iphdr {
#if BYTE_ORDER == LITTLE_ENDIAN
	u_int8_t	ip_hl:4;	/* hdr len */
	u_int8_t	ip_v:4;	/* ver. */
#elif BYTE_ORDER == BIG_ENDIAN
	u_int8_t	ip_v:4;
	u_int8_t	ip_hl:4;
#endif
	u_int8_t	ip_tos;
	u_int16_t	ip_len;	/* packet len */
	u_int16_t	ip_id;	/* pkt id */
	u_int16_t	ip_off;	/* frag. offset */
	u_int8_t	ip_ttl;
	u_int8_t	ip_p;	/* protocol */
	u_int16_t	ip_sum;	/* cksum */
	struct in_addr	ip_src;
	struct in_addr	ip_dst;
#define	IP_DF 0x4000	/* dont frag. */
#define	IP_MF 0x2000	/* more frag. */
};

/* icmp header */

struct _icmphdr {
	u_int8_t	icmp_type;
	u_int8_t 	icmp_code;
	u_int16_t	icmp_cksum;	
		
	/* vstt uses this values to implement reliability */
	u_int16_t	id;	/* number of the pkt */
	u_int16_t	more_follow; /* this used as flag. if it is '1',
			 * then some content of the pkg will
			 * follow within the next pkt.
			 */
};

/* main function definitions */

void run_s2f();
void create_tcp_server();
void create_udp_server();
void create_tcp_client();
void create_udp_client();
void fork_childs();
void err_kill_friends(int, char *);

/* none pseudo protocol */
void init_none();
void init_none_server();
void init_none_client();
void send_none(char *, int);
char *recv_none(char *, int *);

/* pop3 */
void init_pop3();
void init_pop3_server();
void init_pop3_client();
void send_pop3(char *, int);
char *recv_pop3(char *, int *);

/* ip */
void wrapper_rawip_server();
void wrapper_rawip_client();

/* icmp */
void wrapper_send_icmp(char *, int, int16_t);
char *recv_icmp(char *, int *);

/* encryption */
char *vstt_encrypt(char *);

