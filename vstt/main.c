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

/* global connection config */
struct conn_info *ci;
int start_s2f;
int s2f_port;
int s2f_servermode;

enum protocol my_proto;
/* must be equal to enum protocol: */
char *protocols[] = { "icmp", "pop3", "none" };

void
usage(void)
{
	extern char *__progname;
	
	fprintf(stderr,
	    "usage: %s -p proto -r my_port -t peer_port -a peer_addr"
	    " [-m my_addr] [-c <s2f_port> [-s]]\n", __progname);
	fprintf(stderr,
	    "\nPossible values for [protocol] are:\n"
	    "  * none * icmp * pop3\n"
	    "\nThe given port values are ignored when using them\n"
	    "with protocols that doesn't support them (like the\n"
	    "ICMP protocol)\n\n");
	exit(1);
}

void
err_out(char *s)
{
	fprintf(stderr, "%s", s);
	exit(1);
}

void
run_s2f()
{
	char run_str[0xff+1] = { '\0' };
	
	snprintf(run_str, 0xff, S2F_PATH " %s -p %i\n",
		(s2f_servermode == 1 ? "-s" : ""),
		s2f_port);
	
	system(run_str);
	exit(0);
}

int
main(int argc, char *argv[])
{
	int ch;
	int verbose;
	int i;
	char *peer_addr, *my_addr;
	int my_port, peer_port;
	
	my_proto = P_max;
	verbose = 0;
	start_s2f = 0;
	s2f_servermode = 0;
	s2f_port = 0;
	peer_addr = my_addr = NULL;
	
	ci = (struct conn_info *) calloc((size_t)1,
	    sizeof(struct conn_info));
	if (!ci)
		err(1, "calloc");

	my_port = peer_port = 0;
	
	/* setup the internal pointers */
	ci->sa_my = (struct sockaddr_in *) &ci->ss_my;
	ci->sa6_my = (struct sockaddr_in6 *) &ci->ss_my;
	
	ci->sa_peer = (struct sockaddr_in *) &ci->ss_peer;
	ci->sa6_peer = (struct sockaddr_in6 *) &ci->ss_peer;
	
	

	while ((ch = getopt(argc, argv, "hp:r:t:a:m:c:svV")) != -1) {
		switch (ch) {
		case 'p':
			for (i = 0; i < P_max; i++)
				if (strcmp(protocols[i], optarg) == 0)
					my_proto = i;
			break;
		case 'r':
			my_port = atoi(optarg);
			break;
		case 't':
			peer_port = atoi(optarg);
			break;
		case 'c':
			s2f_port = atoi(optarg);
			start_s2f = 1;
			break;
		case 'a':
			if (!(peer_addr = calloc(strlen(optarg) + 1,
			    sizeof(char))))
				err(1, "calloc");
			strncpy(peer_addr, optarg, strlen(optarg));
			break;
		case 'm':
			if (!(my_addr = calloc(strlen(optarg) +1,
			    sizeof(char))))
				err(1, "calloc");
			strncpy(my_addr, optarg, strlen(optarg));
			break;
		case 's':
			s2f_servermode = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'V':
			printf("vstt v. " VSTT_VER " (C) 2016 by "
			"Steffen Wendzel, www. wendzel.de\n");
			exit(0);
			/* NOTREACHED */
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	
/* check s2f parameters */
	if (start_s2f) {
		FILE *fp;
		
		if (s2f_port <= 0 || s2f_port > 0xffff) {
			fprintf(stderr, "s2f port out of range.\n");
			exit(1);
		}
		/* check if s2f is present */
		fp = fopen(S2F_PATH, "r");
		if (!fp)
			err(1, S2F_PATH);
		fclose(fp);
	}
	
/* check vstt parameters */

	if (my_proto == P_pop3)
		printf("please note, that POP3 tunneling is still in "
		    "alpha quality!\n");
	
	if (my_proto == P_max)
		err_out("I miss: '-p proto' or given value isn't "
		    "supported.\n");
	
	if (my_port <= 0 || my_port > 0xffff)
		err_out("I miss: '-r my_port' or port out of range.\n");
	
	if (peer_port <= 0 || peer_port > 0xffff)
		err_out("I miss: '-t peer_port' or port out of "
		    "range.\n");
	
	if (peer_addr == NULL)
		err_out("I miss: '-a peer_address'\n");
	
	if (my_addr == NULL)
		err_out("I miss: '-m my_address'\n");
	
	/* set addr and check+set addr family */
	if (inet_pton(AF_INET, peer_addr,
	    (struct sockaddr_in *) &ci->sa_peer->sin_addr)) {
		ci->family = AF_INET;
		ci->sa_peer->sin_family = AF_INET;
		ci->sa_my->sin_family = AF_INET;
		if (!inet_pton(AF_INET, my_addr,
		    (struct sockaddr_in *) &ci->sa_my->sin_addr))
			err(1, "inet_pton(AF_INET)");
	
	} else if (inet_pton(AF_INET6, peer_addr,
	    (struct sockaddr_in *) &ci->sa6_peer->sin6_addr)) {
		ci->family = AF_INET6;
		ci->sa6_peer->sin6_family = AF_INET6;
		ci->sa6_my->sin6_family = AF_INET6;
		if (!inet_pton(AF_INET6, my_addr,
		    (struct sockaddr_in *) &ci->sa6_my->sin6_addr))
			err(1, "inet_pton(AF_INET6)");
		
	} else {
		fprintf(stderr, "I'm unable to use this address. "
		    "neither inet_pton(AF_INET)\nnor"
		    "inet_pton(AF_INET6, ...) was successfully.\n");
		exit(1);
	}
	
	/* set the ports in the sock* structs */
	if (ci->family == AF_INET) {
		ci->sa_my->sin_port = htons(my_port);
		ci->sa_peer->sin_port = htons(peer_port);
	} else {
		ci->sa6_my->sin6_port = htons(my_port);
		ci->sa6_peer->sin6_port = htons(peer_port);
	}
	
	/* check the protocol support! */
	if (ci->family == AF_INET6) {
		if (my_proto == P_icmp) {
			fprintf(stderr,
			    "ICMP is only support for IPv4. sry\n"
			    "The plan is to change this within\n"
			    "one of the next releases.\n");
			exit(1);
		}
	}
	
	if (verbose) {
		printf("vstt starting ... this is our config:\n");
		printf("protocol     : %s\n", protocols[my_proto]);
		printf("my recv port : %i\n", my_port);
		printf("peer port    : %i\n", peer_port);
		printf("peer address : %s\n", peer_addr);
		printf("proto family : %s\n", (ci->family == AF_INET ?
		    "IPv4" : "IPv6"));
	}
	
	/* check if the proto is already implemented and initialize the
	 * socket base system
	 */
	switch (my_proto) {
	case P_none:
		init_none();
		break;
	case P_pop3:
		init_pop3();
		break;
	case P_icmp:
		//init_icmp();
		break;
	case P_max:
		fprintf(stderr, "Uuupps! something went wrong...\n");
		/* FALLTROUGH */
	default:
		printf("TODO: protocol not implemented\n");
		exit(1);
	}
	fork_childs();
	
	return 0;
}

