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

/* wrapper.c: wrapper functions and such stuff */

#include "main.h"

void sig_handler(int);

extern struct conn_info *ci;
extern enum protocol my_proto;
extern int start_s2f;

int ffd; /* fifo */
int fd[2]; /* pipe for reliability (used in raw socket
            * implementation (ICMP)) */

pid_t pid_child;
pid_t pid_s2f;

/* <Start-Of-Alpha-Quality-Bullshit> */
/* This function is still alpha! */
void
wrapper_tcpserver()
{
	int connfd;
	socklen_t size;
	int len;
	int pkt_len = 0; /* value = 0 not used; just to make gcc happy */
	char buf[16*1024] = {'\0'};
	char *plain_buf;
	char pop3_answbuf[] = "-ERR try next msg.\r\n";
	int counter;
	
	SETSIZE(size);
	while (1) {
		printf("server: waiting for connection...\n");
		fflush(stdout);
		/* accept connection & save client's addr in ss_peer */
		if ((connfd=accept(ci->sockfd_my,
		     (struct sockaddr *)&ci->ss_peer, &size)) < 0)
			err_kill_friends(1, "accept()");
		printf("wrapper_tcpserver: connection established\n");
		FFLUSH
		/* read from socket */
		while ((len = recv(connfd, buf, sizeof(buf)-1, 0)) >
		    0) {
			/* write to output fifo */
			counter = 0;
			pkt_len = len;
			if (my_proto == P_pop3) {
				/* FIXME: If someone sends bullshit, we'll dump core!!! */
				while (buf[counter] != '\0') { 
					plain_buf = recv_pop3(buf+counter, &pkt_len);
					counter+=5;
					while (buf[counter] >= '0' && buf[counter] <= '9') {
						counter++;
					}
					if (buf[counter] == '\r') {
						counter+=2;
						if (plain_buf != NULL) {
#ifdef DEBUG
 							printf("plain_buf = '%s'\n", plain_buf);
#endif
							write(ffd, plain_buf, pkt_len);
							free(plain_buf);
							plain_buf = NULL;
						}
#ifdef XDEBUG
						if (buf[counter] != '\0')
							printf("==(at least) 2nd pkt in ONE==\n");
					} else {
						printf("==bad pkt== (buf=%s)\n", buf+counter);
#endif
					}
				}
			} else if (my_proto == P_none) {
				plain_buf = recv_none(buf, &pkt_len);
			} else {
				fprintf(stderr, "internal error " __FILE__
					":%i\n", __LINE__);
				exit(1);
			}

			if (plain_buf != NULL) {
#ifdef XDEBUG
				printf("plain_buf = '%s'\n", plain_buf);
#endif
				write(ffd, plain_buf, pkt_len);
				free(plain_buf);
			}
			bzero(buf, len);
			
			/* send a response, that looks usualy */
			while(send(connfd, pop3_answbuf, strlen(pop3_answbuf), 0) <= 0) {
				fprintf(stderr, "unable to send response data.\n");
				usleep(500);
			}
		}
		close(connfd);
	}
}
/* </End-of-Alpha-Quality-Bullshit> */

void
wrapper_rawip_server()
{
	char buf[16*1024] = {'\0'};
	char *plain_buf;
	socklen_t size;
	int len;
	int sockfd;
	struct sockaddr_in from;
	
	bzero(&from, sizeof(struct sockaddr_in));
	size = sizeof(struct sockaddr_in);
	
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
		err_kill_friends(1, "socket");

	while ((len = recvfrom(sockfd, buf, sizeof(buf)-1, 0,
	    (struct sockaddr *)&from, &size)) >= 0) {
	   	if (len == 0) {
			continue;
		}
		if (my_proto == P_icmp)
			/* set len too! */
			plain_buf = recv_icmp(buf, &len);
		else {
			fprintf(stderr, "internal error " __FILE__
			    ":%i\n", __LINE__);
			exit(1);
		}
		if (plain_buf != NULL) {
			write(ffd, plain_buf, len);
			free(plain_buf);
			plain_buf = NULL;
		}
		bzero(buf, sizeof(buf));
	}
	err_kill_friends(1, "recvfrom");
}

void
wrapper_tcpclient()
{
	char buf[16*1024] = {'\0'};
	int len;
	
	while (1) {
		/* read data from fifo */
		printf("client: waiting for data from fifo...\n");
		while ((len = read(ffd, buf, sizeof(buf)-1))==0) {
			if (len == -1)
				err(1, "read");
			usleep(100);
		}
		printf("client: read %i bytes from fifo\n", len);
		
		/* send the data we just read out */
		switch (my_proto) {
		case P_pop3:
			send_pop3(buf, len);
			break;
		case P_none:
			send_none(buf, len);
			break;
		case P_icmp: {
			int new = 1;
			int sent = 0;
			char *ptr;
			
			for (ptr = buf; sent < len;) {
				if (new) {
					if (len <= ICMP_PAYLOAD_SIZE) {
						wrapper_send_icmp(ptr,
						    len, 0);
						sent = len;
					} else {
						wrapper_send_icmp(ptr,
						    ICMP_PAYLOAD_SIZE,
						    1/*more*/);
						sent+=ICMP_PAYLOAD_SIZE;
						ptr+=ICMP_PAYLOAD_SIZE;
						new = 0;
					}
				} else {
					/* next part of a pkt */
					if ((len-sent) <=
					    ICMP_PAYLOAD_SIZE) {
						wrapper_send_icmp(ptr,
						    len-sent, 0/*old*/);
						sent=len;
					} else {
						wrapper_send_icmp(ptr,
						    ICMP_PAYLOAD_SIZE,
						    1/*more*/);
						ptr+=ICMP_PAYLOAD_SIZE;
						sent+=ICMP_PAYLOAD_SIZE;
					}
				}
			}
		}
		/* NOTE: len will set back to it's
		 * original value here because we
		 * are leaving the {}-area!
		 */
			break;
		case P_max:
		default:
			printf("Houston...\n");
			exit(1);
		}
		bzero(buf, len);
	}
}

void
wrapper_udpclient()
{
	wrapper_tcpclient();
}

void
wrapper_ipraw_client()
{
	wrapper_tcpclient();
}

void
create_tcp_server()
{
	int yup = 1;
	
	if ((ci->sockfd_my = socket(ci->ss_my.ss_family, SOCK_STREAM,
	    0)) < 0)
		err_kill_friends(1, "socket()");
	
	if (setsockopt(ci->sockfd_my, SOL_SOCKET, SO_REUSEADDR, &yup,
	    sizeof(yup)) != 0)
		err_kill_friends(1, "setsockopt(..., SO_REUSEADDR, ...)");
	
	if (bind(ci->sockfd_my, (struct sockaddr *) &ci->ss_my,
	    (ci->ss_my.ss_family == AF_INET ? sizeof(
	    struct sockaddr_in) : sizeof(struct sockaddr_in6))) < 0)
		err_kill_friends(1, "bind()");

	/* do not use multiple connections, only accept ONE at a time
	 * to make sure we recv()/send() only the right data.
	 */
	if (listen(ci->sockfd_my, 1) < 0)
		err_kill_friends(1, "listen()");
}

void
create_udp_server()
{
	int yup = 1;
	
	if ((ci->sockfd_my = socket(ci->ss_my.ss_family, SOCK_DGRAM,
	    0)) < 0)
		err_kill_friends(1, "socket()");
	
	if (setsockopt(ci->sockfd_my, SOL_SOCKET, SO_REUSEADDR, &yup,
	    sizeof(yup)) != 0)
		err_kill_friends(1, "setsockopt(..., SO_REUSEADDR, ...)");
	
	if (bind(ci->sockfd_my, (struct sockaddr *) &ci->ss_my,
	    (ci->ss_my.ss_family == AF_INET ? sizeof(struct
	    sockaddr_in) : sizeof(struct sockaddr_in6))) < 0)
		err_kill_friends(1, "bind()");
}

void
create_tcp_client()
{
	if ((ci->sockfd_peer = socket(ci->family, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");
}

void create_udp_client()
{
	if ((ci->sockfd_peer = socket(ci->ss_peer.ss_family,
	    SOCK_DGRAM, 0)) < 0)
		err(1, "socket()");
}

void
fork_childs()
{
	pid_s2f = 0;
	pid_child = 0;
	
	if (pipe(fd) < 0)
		perror("pipe");
	
	/* let's create a child */
	pid_child = fork();
	if (pid_child < 0)
		err(1, "fork()");

	if (pid_child == 0) { /* Child == Client */
		/* create the recv_fifo we write into */
		unlink(SEND2PEER_FIFO);
		if (mkfifo(SEND2PEER_FIFO, (mode_t)0660) != 0)
			err(1, "mkfifo()");
		if ((ffd = open(SEND2PEER_FIFO, O_RDWR, 0)) == -1)
			err(1, "open " SEND2PEER_FIFO);
		
		/* if reliability is needed: use pipes,
		 * if not: don't care
		 */
		close(fd[1]);
		if (fcntl(fd[0], F_SETFL, O_NONBLOCK) == -1)
			err(1, "fcntl");
		
		/* init the protocol client */
		switch (my_proto) {
		case P_pop3:
			init_pop3_client();
			wrapper_tcpclient();
			break;
		case P_none:
			init_none_client();
			wrapper_tcpclient();
			break;
		case P_icmp:
			wrapper_ipraw_client();
		case P_max:
		default:
			printf("Houston...\n");
			exit(1);
			break;
		}
	} else { /* Parent == Server */
		/* create the send_fifo we read from */
		unlink(RECVFPEER_FIFO);
		if (mkfifo(RECVFPEER_FIFO, (mode_t)0660) != 0)
			err(1, "mkfifo()");
		/* linux pipe stuff is broken, thats why I use O_RDWR */
		if ((ffd = open(RECVFPEER_FIFO, O_RDWR, 0)) == -1)
			err(1, "open " RECVFPEER_FIFO);
		
		/* if reliability is needed: use pipes,
		 * if not: don't care
		 */
		close(fd[0]);
		
		/* init signal handlers for the server (=parent) */
		if (signal(SIGINT, sig_handler) == SIG_ERR)
			err_kill_friends(1, "signal");
                if (signal(SIGTERM, sig_handler) == SIG_ERR)
			err_kill_friends(1, "signal");
		if (signal(SIGCHLD, sig_handler) == SIG_ERR)
			err_kill_friends(1, "signal");
		
		/* start a s2f process */
		
		if (start_s2f) {
			pid_s2f = fork();
			if (pid_s2f < 0)
				err(1, "fork()");
			if (pid_s2f == 0) /* child starts s2f */
				run_s2f();
		}
		
		/* init the protocol server */
		switch (my_proto) {
		case P_pop3:
			init_pop3_server();
			wrapper_tcpserver();
			break;
		case P_none:
			init_none_server();
			wrapper_tcpserver();
			break;
		case P_icmp:
			wrapper_rawip_server();
			break;
		case P_max:
		default:
			printf("Houston...\n");
			exit(1);
		}
	}
}

/* kill s2f and vstt-child and print err() */

void
err_kill_friends(int eval, char *str)
{
	/* check if the pid-values are zero. in this case fork() made
	 * problems or the function is called from the client!
	 *  => don't kill child proccesses if we are a client!
	 */
	if (pid_child)
		kill(pid_child, SIGKILL);
	
	if (pid_s2f)
		kill(pid_s2f, SIGKILL);
	
	err(eval, "%s", str);
}

void sig_handler(int signo)
{
	fprintf(stderr, "received signal=%i\n", signo);
	err_kill_friends(1, "sig_handler called");
	exit(0);
}

