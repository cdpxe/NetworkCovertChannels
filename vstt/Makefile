#/* This is the vstt Sourcecode - http://wendzel.de
# *
# * Copyright (c) 2006 and 2016 Steffen Wendzel, www.wendzel.de
# *       - All rights reserved.
#  *
#  * Redistribution and use in source and binary forms, with or without
#  * modification, are permitted provided that the following conditions
#  * are met:
#  * 1. Redistributions of source code must retain the above copyright
#  *    notice, this list of conditions and the following disclaimer.
#  * 2. Redistributions in binary form must reproduce the above copyright
#  *    notice, this list of conditions and the following disclaimer in the
#  *    documentation and/or other materials provided with the distribution.
#  *
#  * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
#  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
#  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
#  * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#  * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#  * SUCH DAMAGE.
#  *
#  */

CC=	gcc
# DON'T turn on -DDEBUG if you don't know what it does ;-)
#DEBUG=	-g -ggdb -DDEBUG
CFLAGS=	$(DEBUG) -W -Wall -Wshadow -c
OBJETS=	main.o wrapper.o pop3.o none.o ip_raw.o
LIBS=	

.SUFFIXES : .c .o

.c.o :
	$(CC) ${CFLAGS} $<

all : vstt reader s2f
	gcc $(DEBUG) -O2 -o vstt $(OBJETS)

solaris : vstt reader s2f_solaris
	gcc $(DEBUG) -O2 -o vstt $(OBJETS) -lnsl -lsocket

#vstt2 is for local debugging only
vstt2 : $(OBJETS)
	gcc -O -DSEND2PEER_FIFO=\"/tmp/.vstt_send2peer2\" -DRECVFPEER_FIFO=\"/tmp/.vstt_recvfpeer2\" -o vstt2 $(OBJETS)

vstt : $(OBJETS)

s2f : s2f.o
	gcc -O2 -o s2f s2f.o

s2f_solaris : s2f.o
	gcc -O2 -o s2f s2f.o -lnsl -lsocket

reader : reader.o
	gcc -O -o reader reader.o

clean :
	rm -f *.o *~ vstt vstt2 reader s2f *.core core a.out

count :
	if [ ! -f /bsd ]; then wc -l *.[ch] | sort -bg; else wc -l *.[ch] | sort; fi

install :
	cp vstt s2f /usr/sbin/


