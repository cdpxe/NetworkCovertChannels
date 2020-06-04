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

/* none (pseudo protocol) functions */

#include "main.h"
#include <inttypes.h>

extern struct conn_info *ci;

void
init_none()
{
	create_tcp_server();
	create_tcp_client();
}

void
init_none_client()
{
/*	int size;*/
	int ret = 0;
/*	void *ptr;
	
	SETSIZE(size)
	
	if (ci->family == AF_INET)
		ptr = ci->sa_peer;
	else
		ptr = ci->sa6_peer;
*/	
	puts("client: connecting to peer ...");
	do {
		if (ret == -1) {
			perror("none(or pop3 and so on)_client: "
			    "connect()");
			close(ci->sockfd_peer);
			create_tcp_client();
		}
		sleep(1);
	} while ((ret=connect(ci->sockfd_peer,
	    (struct sockaddr *)ci->sa_peer,
		  sizeof(struct sockaddr_in)))!=0);
	printf("==> con established\n");
	fflush(stdout);
}

void
init_none_server()
{
	/* nothing to do here */
}

void
send_none(char *buf, int len)
{
	if (!send(ci->sockfd_peer, buf, len, 0))
		perror("send_none: send()");
}

char *
recv_none(char *input, int *len)
{
	char *buf;
	
	printf("DEBUG: len=%i", *len); fflush(stdout);
	
	buf = (char *) calloc(*len + 1, sizeof(char));
	if (!buf)
		err_kill_friends(1, "calloc()");
	memcpy(buf, input, *len);
	return buf;
}

