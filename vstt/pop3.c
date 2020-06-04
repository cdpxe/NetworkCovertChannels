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

/* pop3 functions */

#include "main.h"

extern struct conn_info *ci;

void
init_pop3()
{
	init_none();
}

void
init_pop3_client()
{
	init_none_client();
}

void
init_pop3_server()
{
	init_none_server();
}

void
send_pop3(char *buf, int len)
{
	int	 i;
	char	 lbuf[32];
	int	 num;
	
	printf("send_pop3().input = '%s'\n", buf);
	
	/* first send the len of the pkt in bytes */
	snprintf(lbuf, sizeof(lbuf), "RETR %i\r\n", len);
	printf("sending number of bytes: '%s'\n", lbuf);
	if(!send(ci->sockfd_peer, lbuf, strlen(lbuf), 0))
		err_kill_friends(1, "send");
	
	/* now send the bytes */
	printf("client: sending ");
	for (i=0; i < len; i++) {
		bzero(lbuf, sizeof(lbuf));
		strncpy(lbuf, "RETR ", 5);
		
		num = buf[i];
		snprintf(lbuf+5, 24, "%i\r\n", num);

		printf("%s", lbuf+5);
		if(!send(ci->sockfd_peer, lbuf, strlen(lbuf), 0))
			err_kill_friends(1, "send");
		
		/* FIXME: the server never sends more than
		 * sizeof(buf)-1 bytes ;)
		 */
		if(!recv(ci->sockfd_peer, lbuf, sizeof(lbuf)-1, 0))
			err_kill_friends(1, "recv");
	}
}

char *
recv_pop3(char *input, int *len)
{
	static int	 pkt_len = 0;
	static int	 cur_len = 0;
	static char	*buf = NULL;
	int		 num;
	
	printf("%i,", cur_len);
	
	if (pkt_len == 0) {
		buf = NULL;
		/* recv the pkt len */
		pkt_len = atoi(input + 5);
	} else {
		cur_len++;
		if (!(buf = (char *) realloc(buf, cur_len+1)))
			err_kill_friends(1, "realloc");
		buf[cur_len] = '\0';
		num = atoi(input + 5);
		buf[cur_len - 1] = (char) num;
		
		if (cur_len == pkt_len) {
#ifdef DEBUG
			printf("\n================================================\n");
			printf("pkt_len = %i, strlen() = %i\n", pkt_len, (int)strlen(buf));
			printf("buf = '%s'\n", buf);
			printf("================================================\n");
#endif
			*len = pkt_len;
			pkt_len = 0;
			cur_len = 0;
			return buf;
		}
	}	
	return NULL;
}

