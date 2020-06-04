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

int sockfd;
int connfd;
socklen_t size;
struct sockaddr_in sa;
int port;
int i_am_a_server;
int yup;

void
usage()
{
	extern char *__progname;
	
	fprintf(stderr, "usage: %s [-s] [-i input-fifo] "
	    "[-o output-fifo] -p port\n", __progname);
	exit(1);
}

void
do_connect()
{
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");
	
	bzero(&sa, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	
	if (i_am_a_server) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		    &yup, sizeof(yup)) != 0)
			err(1, "setsockopt(SO_REUSEADDR)");
		
		sa.sin_addr.s_addr = INADDR_ANY;
		
		if (bind(sockfd, (struct sockaddr *) &sa,
		    sizeof(struct sockaddr_in)) < 0)
			err(1, "bind()");
	
		/* do not use multiple connections, only accept ONE at
		 * a time to make sure we recv()/send() only the right
		 * data.
		 */
		if (listen(sockfd, 1) < 0)
			err(1, "listen()");
			
		size = sizeof(struct sockaddr_in);
		if ((connfd=accept(sockfd, (struct sockaddr *) &sa,
		    &size)) < 0)
			err(1, "accept()");
		printf("s2f ready.\n");
	} else {
		if (!inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr))
			err(1, "inet_pton");
		
		while (connect(sockfd, (struct sockaddr *) &sa,
		    sizeof(struct sockaddr_in)) == -1) {
			perror("connect");
			close(sockfd);
			if ((sockfd = socket(AF_INET, SOCK_STREAM, 0))
			    < 0)
				err(1, "socket()");
			sleep(1);
		}
		printf("connected.\n");
		/* play it on this way */
		connfd = sockfd;
	}
}

int
main(int argc, char *argv[])
{
	char *input;
	char *output;
	int ch;
	int iffd, offd;
	struct sockaddr name;
	socklen_t namelen;
	fd_set fds;
	char buf[16*1024] = { '\0' };
	int len, slen;
	int peak;
#ifdef DEBUG
	int incoming_pkts = 0;
	int sent_pkts = 0;
#endif
	
	input = output = NULL;
	port = i_am_a_server = 0;
	yup = 1;
	namelen = sizeof(name);

	while ((ch = getopt(argc, argv, "hi:o:p:s")) != -1) {
		switch (ch) {
		case 'i':
			if (!(input = (char *) calloc(strlen(optarg)+1,
			    sizeof(char))))
				err(1, "calloc()");
			strncpy(input, optarg, strlen(optarg));
			break;
		case 'o':
			if (!(output = (char *)
			    calloc(strlen(optarg)+1, sizeof(char))))
				err(1, "calloc()");
			strncpy(output, optarg, strlen(optarg));
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 's':
			i_am_a_server = 1;
			break;
		case 'h':
			/* FALLTROUGH */
		default:
			usage();
			/* NOTREACHED */
		}
	}
	
	if (input == NULL)
		input = RECVFPEER_FIFO;
	if (output == NULL)
		output = SEND2PEER_FIFO;
	
	if (port == 0)
		usage();
	
/* fifo setup */
	if ((iffd = open(input, O_RDWR)) == -1)
		err(1, "open(input)");
	
	if ((offd = open(output, O_RDWR)) == -1)
		err(1, "open(output)");
	
/* network setup */
	do_connect();
	
/* select setup */
	FD_ZERO(&fds);

/* mainloop */
	do {
		if (getpeername(connfd, &name, &namelen) == -1) {
			if (errno == ENOTCONN || errno == EBADF
			|| ENOTSOCK) {
				/* socket closed || not connected
				 * => re-connect */
				printf("s2f: connection closed. re-connecting.\n");
				do_connect();
			} else {
				err(1, "s2f: getpeername");
			}
		}
	
		FD_SET(connfd, &fds);
		FD_SET(iffd, &fds);
		peak = (connfd > iffd ? connfd : iffd);
		if (select(peak+1, &fds, NULL, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			else
				err(1, "select");
		}
		
		if (FD_ISSET(iffd, &fds)) {
			// read the buf
			len = read(iffd, buf, sizeof(buf)-1);
			if (len < 0)
				err(1, "read");
			// send the buf
			if (len) {
				if (!(slen = send(connfd, buf, len, 0)))
					err(1, "send");
#ifdef DEBUG
				printf("recv: %i: len=%i  , slen=%i\n",
				    ++incoming_pkts, len, slen);
				FFLUSH
#endif
			}
			bzero(buf, len);
		}
		if (FD_ISSET(connfd, &fds)) {
			// read from socket
			len = recv(connfd, buf, sizeof(buf)-1, 0);
			if (len < 0)
				err(1, "recv");
			
			// write the buf
			if (len) {
				if (!(slen = write(offd, buf, len)))
					err(1, "write");
#ifdef DEBUG
				printf("send: %i: len=%i\n",
				    ++sent_pkts, len);
				FFLUSH
#endif
			}
			bzero(buf, len);
		}
	} while (1);
	
	return 0;
}
