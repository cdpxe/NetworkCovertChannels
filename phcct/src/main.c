/* Copyright (c) 2007 Steffen Wendzel <steffen (at) wendzel (dot) de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 * THIS TOOL IS FOR LEGAL PURPOSES ONLY!
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/tcp.h>

#define PHCCT_VERSION		"0.1.1"
#define PHCCT_PATCHLEVEL	"p0"
#define PHCCT_VER		PHCCT_VERSION PHCCT_PATCHLEVEL

#define LOCAL_PORT		9999
/* worked fine on MTU=1500 */
#define MAX_READ_SIZE		100

#define MAGIC_STRING		"FW=0:"
#define HTTP_MAGIC_IDSTR	"Cookie: GPC="
#define FTPDATA_MAGIC_IDSTR	"\1\2\3"
#define PLAINTEXT_MAGIC_IDSTR	FTPDATA_MAGIC_IDSTR

struct recv_data {
	u_int32_t	  id;
	void		 *data;
	struct recv_data *next, *prev;
};

typedef struct recv_data recv_data;

struct timeval s, e;

void *calloc_exit(int size, int elem);

extern int errno;

char *peer_addr;
u_int8_t num_protos = 0;
recv_data *slist = NULL;
int conn_estab = 0;
int connfd;

void
usage(void)
{
	extern char *__progname;
	
	fprintf(stderr,
		"usage: %s -V | -a peer_addr\n",
		__progname);
	exit(1);
}

/* This function is from 
 * http://haifux.org/lectures/132/stable-recurring-timer.c
 */
int time_diff(struct timeval *t1, struct timeval *t2, struct timeval *diff)
{
    /* sanity check. */
    if (t1->tv_sec > t2->tv_sec ||
        (t1->tv_sec == t2->tv_sec && t1->tv_usec > t2->tv_usec))
        return -1;

    if (t2->tv_sec == t1->tv_sec) {
        diff->tv_sec = 0;
        diff->tv_usec = t2->tv_usec - t2->tv_usec;
    }
    else {
        diff->tv_sec = t2->tv_sec - t1->tv_sec;
        if (t2->tv_usec >= t1->tv_usec)
            diff->tv_usec = t2->tv_usec - t1->tv_usec;
        else {
            diff->tv_usec = 1000000 + t2->tv_usec - t1->tv_usec;
            diff->tv_sec -= 1;
        }
    }

    return 0;
}


void
add2slist(int id, char *payld)
{
	recv_data *rd;
	recv_data *ptr;
	void *data;

	if (id < 0) {
		printf("id < 0\n");
		exit(1);
	}

	/* since payld is a char buf[] we have to create a copy first */
	data = calloc_exit(strlen(payld) + 1, sizeof(char));
	memcpy(data, payld, strlen(payld));

	rd = (recv_data *) calloc_exit(1, sizeof(recv_data));
	rd->id = id;
	rd->data = data;
	rd->next = rd->prev = NULL;

	if (!slist) {
		slist = rd;
	} else {
		/* this is time consuming on output function since I
		 * do not sort this slist */
		for (ptr = slist; ptr->next; ptr = ptr->next)
			;
		ptr->next = rd;
		rd->prev = ptr;
	}
}




void
output_slist()
{
	recv_data *ptr;
	static u_int32_t wanted_id = 0;
	int id_found = 1;

	/* wait for connection */
	if (!conn_estab)
		return;

	if (!slist)
		return;

	while (id_found) {
		id_found = 0;
		for (ptr = slist; !id_found && ptr != NULL; ) {
			if (ptr->id == wanted_id) {
				/* output ptr->data */
				send(connfd, ptr->data, strlen(ptr->data), 0);
				//fprintf(stderr, "%s\n", (char *) ptr->data);
				id_found = 1;
				wanted_id++;

				free(ptr->data);

				if (ptr->prev) {
					ptr->prev->next = ptr->next;
					if (ptr->next)
						ptr->next->prev = ptr->prev;
				}
				
				if (ptr == slist) {
					slist = slist->next;
					if (slist)
						slist->prev = NULL;
				}
				
				free(ptr);
				ptr = NULL;
			}
			if (ptr != NULL)
				ptr = ptr->next;
		}
	}
}

void
inform_disconnected()
{
	fprintf(stderr, "The connection was closed by foreign host.\n");
	fprintf(stderr, "Shutdown ..\n");
	exit(0);
}

void
inform_nomagicstr(char *dbg_buf)
{
	u_int32_t i;
	char c;

	printf("Received pkt without magic (id) string. Here is a dump:\n");
	for (i = 0; i < strlen(dbg_buf); i++) {
		c = dbg_buf[i];
		printf("%2x [%c] ", c, (c == '\r' || c == '\n' ? ' ' : c));
		if (i % 10 == 9)
			putchar('\n');
	}
	putchar('\n');
	exit(1);
	
}

void *
recv_thread(/*ARGSUSED0*/ void *unused)
{
	int sockfd_http, sockfd_ftpdata, sockfd_plain;
	struct sockaddr_in sa_http, sa_ftpdata, sa_plain;
	int connfd_http, connfd_ftpdata, connfd_plain;
	socklen_t salen;
	int yup = 1;
	int ret = 0;
	int id;
	fd_set rset;
	int max = 0;
	char buf[0xffff];
	char *idptr, *payld;
	int x;

	salen = sizeof(struct sockaddr_in);
	connfd_http = connfd_ftpdata = connfd_plain = -1;

	/* create our own sockets and open our ports */
	if ((sockfd_http = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");
	if ((sockfd_ftpdata = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");
	if ((sockfd_plain = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");

	if (setsockopt(sockfd_http, SOL_SOCKET, SO_REUSEADDR, &yup,
			sizeof(yup)) != 0)
		err(1, "setsockopt(..., SO_REUSEADDR, ...)");
	if (setsockopt(sockfd_ftpdata, SOL_SOCKET, SO_REUSEADDR, &yup,
			sizeof(yup)) != 0)
		err(1, "setsockopt(..., SO_REUSEADDR, ...)");
	if (setsockopt(sockfd_plain, SOL_SOCKET, SO_REUSEADDR, &yup,
			sizeof(yup)) != 0)
		err(1, "setsockopt(..., SO_REUSEADDR, ...)");

	bzero(&sa_http, sizeof(struct sockaddr_in));
	bzero(&sa_ftpdata, sizeof(struct sockaddr_in));
	bzero(&sa_plain, sizeof(struct sockaddr_in));

	sa_http.sin_family = AF_INET;
	sa_http.sin_port = htons(80);
	sa_http.sin_addr.s_addr = INADDR_ANY;

	sa_ftpdata.sin_family = AF_INET;
	sa_ftpdata.sin_port = htons(20);
	sa_ftpdata.sin_addr.s_addr = INADDR_ANY;

	sa_plain.sin_family = AF_INET;
	sa_plain.sin_port = htons(2510);
	sa_plain.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd_http, (struct sockaddr *) &sa_http,
				sizeof(struct sockaddr_in)) < 0)
		err(1, "bind()");
	if (bind(sockfd_ftpdata, (struct sockaddr *) &sa_ftpdata,
				sizeof(struct sockaddr_in)) < 0)
		err(1, "bind()");
	if (bind(sockfd_plain, (struct sockaddr *) &sa_plain,
				sizeof(struct sockaddr_in)) < 0)
		err(1, "bind()");

	if (listen(sockfd_http, 1) < 0) err(1, "listen()");
	if (listen(sockfd_ftpdata, 1) < 0) err(1, "listen()");
	if (listen(sockfd_plain, 1) < 0) err(1, "listen()");

	if ((connfd_http = accept(sockfd_http, (struct sockaddr *) &sa_http,
				   &salen)) < 0) {
		err(1, "accept(http)");
	}
	if ((connfd_ftpdata = accept(sockfd_ftpdata, (struct sockaddr *)
					&sa_ftpdata, &salen)) < 0) {
		err(1, "accept(ftpdata)");
	}
	if ((connfd_plain = accept(sockfd_plain, (struct sockaddr *) &sa_plain,
				   &salen)) < 0) {
		err(1, "accept(plain)");
	}

	max = connfd_http;
	if (connfd_ftpdata > max) max = connfd_ftpdata;
	if (connfd_plain > max) max = connfd_plain;
	
	for (;;) {
		FD_ZERO(&rset);
		FD_SET(connfd_http, &rset);
		FD_SET(connfd_ftpdata, &rset);
		FD_SET(connfd_plain, &rset);
		
		ret = select(max + 1, &rset, NULL, NULL, NULL);
		if (ret) {
			bzero(buf, sizeof(buf));

			if (FD_ISSET(connfd_http, &rset)) {
				if ((x = recv(connfd_http, buf,
						sizeof(buf) - 1, 0)) == -1)
					err(1, "recv(http)");
				if (x == 0)
					inform_disconnected();
				if ((idptr = strstr(buf, HTTP_MAGIC_IDSTR)) == NULL) {
					inform_nomagicstr(buf);
					continue;
				}
				idptr += strlen(HTTP_MAGIC_IDSTR);
				if ((payld = strstr(buf, MAGIC_STRING)) == NULL) {
					/* something went wrong... did domeone send
					 * http to our "server"? */
					inform_nomagicstr(buf);
					continue;
				}
				payld += strlen(MAGIC_STRING);
				id = atoi(idptr);
				if (id < 0) {
					printf("recv on http buf: id < 0");
					exit(1);
				}
				/* cut trailing \r\n\r\n */
				payld[strlen(payld) - 1 - 3] = '\0';

				//printf("http-extracted-id=%i\n", id);
				//printf("http payload: %s\n", payld);

				// add to slist
				add2slist(id, payld);
			} else if (FD_ISSET(connfd_ftpdata, &rset)) {
				if ((x = recv(connfd_ftpdata, buf,
						sizeof(buf) - 1, 0)) == -1)
					err(1, "recv(ftpdata)");
				if (x == 0)
					inform_disconnected();
				if ((idptr = strstr(buf, FTPDATA_MAGIC_IDSTR)) == NULL) {
					inform_nomagicstr(buf);
					continue;
				}
				idptr += strlen(FTPDATA_MAGIC_IDSTR);
				if ((payld = strstr(buf, MAGIC_STRING)) == NULL) {
					inform_nomagicstr(buf);
					continue;
				}
				payld += strlen(MAGIC_STRING);
				id = atoi(idptr);
				if (id < 0) {
					printf("recv on ftpdata buf: id < 0");
					exit(1);
				}
				
				// add to slist
				add2slist(id, payld);
			} else if (FD_ISSET(connfd_plain, &rset)) {
 				if ((x = recv(connfd_plain, buf,
						sizeof(buf) - 1, 0)) == -1)
					err(1, "recv");
				if (x == 0)
					inform_disconnected();
				if ((idptr = strstr(buf, PLAINTEXT_MAGIC_IDSTR)) == NULL) {
					inform_nomagicstr(buf);
					continue;
				}
				idptr += strlen(PLAINTEXT_MAGIC_IDSTR);
				if ((payld = strstr(buf, MAGIC_STRING)) == NULL) {
					inform_nomagicstr(buf);
					continue;
				}
				payld += strlen(MAGIC_STRING);
				id = atoi(idptr);
				if (id < 0) {
					printf("recv on plaintext buf: id < 0");
					exit(1);
				}
				
				// add to slist
				add2slist(id, payld);
			} else {
				printf("huh?!\n");
				exit(1);
			}
			output_slist();
		} else if (ret == 0) {
			/* do nothing */
		} else { /* ret = -1 */
			if (errno == EINTR)
				continue;
			else
				err(1, "select");
		}
	}
	/*NOTREACHED*/
	return NULL;
}

void *
calloc_exit(int size, int elem)
{
	void *q;
	
	if (!(q = calloc(size, elem))) {
		perror("calloc");
		printf("call was calloc_exit(%i, %i)\n", size, elem);
		exit(1);
	}
	return q;
}

int
get_rand(int max)
{
	return rand() % max;
}

int
main(int argc, char *argv[])
{
	socklen_t salen = sizeof(struct sockaddr_in);
	int ch;
	int ret = 0;
	int yup = 1;
	int ffd; /* main connection socket for listen() */
	pthread_t pt_recvt;
#ifndef __linux__
	pthread_attr_t attr;
#endif
	int sockfd_http, sockfd_ftpdata, sockfd_plain;
	struct sockaddr_in sa_http, sa_ftpdata, sa_plain;
	struct sockaddr_in sa_local; /* client connects to that */
	u_int32_t next_id = 0;
	char rbuf[MAX_READ_SIZE];
	int first_pkt_sent = 0;

	peer_addr = NULL;

	while ((ch = getopt(argc, argv, "a:Vh")) != -1) {
		switch (ch) {
		case 'a':
			if (!(peer_addr = (char *) calloc_exit(strlen(optarg) + 1,
							  sizeof(char))))
				err(1, "calloc");
			strncpy(peer_addr, optarg, strlen(optarg));
			break;
		case 'V':
			printf("phcct v. " PHCCT_VER " (C) "
				"2007 Steffen Wendzel <steffen (at) "
				"wendzel (dot) de>.\n");
			printf("Visit http://www.wendzel.de/\n");
			exit(0);
			/* NOTREACHED */
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	printf("starting phcct ...\n");
	
	if (!peer_addr) {
		fprintf(stderr, "need peer address.\n");
		usage();
	}
	
	/* socket for client connection */
	if ((ffd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "main listen socket()");

	bzero(&sa_local, sizeof(struct sockaddr_in));

	sa_local.sin_family = AF_INET;
	sa_local.sin_port = htons(LOCAL_PORT);
	sa_local.sin_addr.s_addr = INADDR_ANY;

	if (setsockopt(ffd, SOL_SOCKET, SO_REUSEADDR, &yup,
			sizeof(yup)) != 0)
		err(1, "setsockopt(..., SO_REUSEADDR, ...)");

	if (bind(ffd, (struct sockaddr *) &sa_local,
				sizeof(struct sockaddr_in)) < 0)
		err(1, "bind()");

	if (listen(ffd, 1) < 0) err(1, "listen()");
	
	/* start the receiver thread */
#ifndef __linux__
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 99999);
#endif
	if (pthread_create(&pt_recvt,
#ifndef __linux__
		&attr,
#else
		NULL,
#endif
		&recv_thread, NULL) != 0)
		err(1, "pthread_create");

	/* create our own sockets and open our ports */
	if ((sockfd_http = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");
	num_protos++;
	if ((sockfd_ftpdata = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");
	num_protos++;
	if ((sockfd_plain = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(1, "socket()");
	num_protos++;

	bzero(&sa_http, sizeof(struct sockaddr_in));
	bzero(&sa_ftpdata, sizeof(struct sockaddr_in));
	bzero(&sa_plain, sizeof(struct sockaddr_in));

	sa_http.sin_family = AF_INET;
	sa_http.sin_port = htons(80);
	if (!inet_pton(AF_INET, peer_addr, &sa_http.sin_addr))
		err(1, "inet_pton");
	sa_ftpdata.sin_family = AF_INET;
	sa_ftpdata.sin_port = htons(20);
	if (!inet_pton(AF_INET, peer_addr, &sa_ftpdata.sin_addr))
		err(1, "inet_pton");
	sa_plain.sin_family = AF_INET;
	sa_plain.sin_port = htons(2510);
	if (!inet_pton(AF_INET, peer_addr, &sa_plain.sin_addr))
		err(1, "inet_pton");

	printf("please press return if the peer is setup up.");
	fgetc(stdin);
	printf("connecting ...\n");
	
	/* connect all these sockets */
	if (connect(sockfd_http, (struct sockaddr *) &sa_http,
				sizeof(struct sockaddr_in)) != 0)
		err(1, "connect");
	printf("connected via http\n");
	if (connect(sockfd_ftpdata, (struct sockaddr *) &sa_ftpdata,
				sizeof(struct sockaddr_in)) != 0)
		err(1, "connect");
	printf("connected via ftp-data\n");
	if (connect(sockfd_plain, (struct sockaddr *) &sa_plain,
				sizeof(struct sockaddr_in)) != 0)
		err(1, "connect");
	printf("connected via plain proto\n");

	printf("setting socket options ... ");
	if (setsockopt(sockfd_http, IPPROTO_TCP, TCP_NODELAY, &yup,
			sizeof(yup)) != 0)
		err(1, "setsockopt(..., IPPROTO_TCP, TCP_NODELAY ...)");
	if (setsockopt(sockfd_ftpdata, IPPROTO_TCP, TCP_NODELAY, &yup,
			sizeof(yup)) != 0)
		err(1, "setsockopt(..., IPPROTO_TCP, TCP_NODELAY ...)");
	if (setsockopt(sockfd_plain, IPPROTO_TCP, TCP_NODELAY, &yup,
			sizeof(yup)) != 0)
		err(1, "setsockopt(..., IPPROTO_TCP, TCP_NODELAY ...)");
	printf("done.\n");

	
	srand(time(NULL));

	printf("waiting for local connection on port %i ...", LOCAL_PORT);
	fflush(stdout);
	if ((connfd = accept(ffd, (struct sockaddr *) &sa_local, &salen)) < 0)
		err(1, "accept(local client)");
	conn_estab = 1;
	printf(" connection accepted.\n");

	/* sender mainloop */
	for (;;) {
		int len;
		char *sbuf;
		
		bzero(rbuf, sizeof(rbuf));
		
		if ((ret = recv(connfd, rbuf, sizeof(rbuf) - 1, 0)) == -1)
			err(1, "read");
		if (ret == 0) {
			struct timeval diff;

			perror("local connection shutdown\n");
			gettimeofday(&e, NULL);
			time_diff(&s, &e, &diff);
			printf("Time difference: ");
			printf("%li.%lisec\n", diff.tv_sec, diff.tv_usec);
			exit(1);
		}

		len = strlen(rbuf);

		if (first_pkt_sent == 0) {
			gettimeofday(&s, NULL);
			first_pkt_sent = 1;
		}

		switch (get_rand(num_protos)) {
		case 0:
			/* http */
			printf("sending via http...\n");
			sbuf = (char *) calloc_exit(len + 0xff + 1,
							sizeof(char));
			snprintf(sbuf, len + 0xff - 1,
				"GET / HTTP/1.1\r\n"
				"Host: google.de\r\n"
				"User-Agent: Mozilla/5.0\r\n"
				"Accept: text/xml\r\n"
				"Accept-Language: en-us;q=0.5,en;q=0.3\r\n"
				"Accept-Encoding: gzip,deflate\r\n"
				"Accept-Charset: ISO-8859-1,utf-8\r\n"
				"Keep-Alive: 300\r\n"
				"Connection: keep-alive\r\n"
				HTTP_MAGIC_IDSTR "%i"
				MAGIC_STRING "%s\r\n\r\n",
				next_id, rbuf);
			printf("   %i bytes\n", strlen(sbuf));

			if (send(sockfd_http, sbuf, strlen(sbuf), 0) < 0)
				err(1, "send(http)");
			free(sbuf);
			break;
		case 1:
			/* ftpdata */
			printf("sending via ftpdata...\n");
			sbuf = (char *) calloc_exit(len + 0xff + 1,
							sizeof(char));
			snprintf(sbuf, len + 0xff - 1,
				FTPDATA_MAGIC_IDSTR "%i " MAGIC_STRING "%s",
				next_id, rbuf);
			printf("   %i bytes\n", strlen(sbuf));
			if (send(sockfd_ftpdata, sbuf, strlen(sbuf), 0) < 0)
				err(1, "send(ftpdata)");
			free(sbuf);
			break;
		case 2:
			/* plaintext */
			printf("sending via port 2510...\n");
			sbuf = (char *) calloc_exit(len + 0xff + 1,
							sizeof(char));
			snprintf(sbuf, len + 0xff - 1,
				PLAINTEXT_MAGIC_IDSTR "%i " MAGIC_STRING "%s",
				next_id, rbuf);
			printf("   %i bytes\n", strlen(sbuf));
			if (send(sockfd_plain, sbuf, strlen(sbuf), 0) < 0)
				err(1, "send(plaintext)");
			free(sbuf);
			break;
		default:
			printf("unsupported protocol in mainloop!\n");
			exit(1);
			/*NOTREACHED*/
			break;
		}
		next_id++;
		usleep(10000);
	}

	/*NOTREACHED*/
	return 0;
}



