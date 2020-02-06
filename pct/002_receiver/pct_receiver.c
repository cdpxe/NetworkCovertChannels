/* pct_receiver.c, a receiver tool for protocol channel messages
 * (receives messages sent by the tool 'pct_sender.pl').
 * (C) 2008 Steffen Wendzel, <steffen (at) wendzel (dot) de>
 *      http://www.wendzel.de
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <pcap.h>
#include <stdlib.h>
#include <err.h>
#include <signal.h>

#define FRAME_LEN	500
#define BUFFERSIZ	512
#define PCTYP_ARP	0
#define PCTYP_ICMP	1

char plaintext_buf[BUFFERSIZ] = { '\0' };
short plaintext_buf_pos = 0;

pcap_t *SetIf(char *);
void SigHandler(int);
char ExtractCode(short int[6]);
void NewPkt(short);

/* Prepare interface settings for libpcap */
pcap_t *
SetIf(char *dev)
{
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *descr;
  struct bpf_program filter;

  if (!(descr = pcap_open_live(dev, FRAME_LEN, 1, 100, errbuf)))
    err(1, "%s\n", errbuf);

  /* Compile Pcap-Filter to only accept ICMP and ARP */
  if (pcap_compile(descr, &filter,
        "arp or (icmp and icmp[icmptype] = 8)", 0, 0) == -1)
    err(1, "pcap_compile error\n");

  if (pcap_setfilter(descr, &filter))
    err(1, "pcap_setfilter error\n");
  return descr;
}

/* Re-create the original content based on the protocol
 * information received. Also do a parity check here.
 */
char
ExtractCode(short int recv_buf[6])
{
  /* 'A' => "00000", 'B' => "00001", 'C' => "00010", 'D' => "00011",
     'E' => "00100", 'F' => "00101", 'G' => "00110", 'H' => "00111",
     'I' => "01000", 'J' => "01001", 'K' => "01010", 'L' => "01011",
     'M' => "01100", 'N' => "01101", 'O' => "01110", 'P' => "01111",
     'Q' => "10000", 'R' => "10001", 'S' => "10010", 'T' => "10011",
     'U' => "10100", 'V' => "10101", 'W' => "10110", 'X' => "10111",
     'Y' => "11000", 'Z' => "11001", ' ' => "11010", '_' => "11011",
     '-' => "11100", '$' => "11101", '.' => "11110", ',' => "11111"); */
  static char codes[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ _-$.,";
  short val = 0;
  short zero_bits = 5;

  if (recv_buf[0]) { val += 16; zero_bits--; }
  if (recv_buf[1]) { val += 8; zero_bits--; }
  if (recv_buf[2]) { val += 4; zero_bits--; }
  if (recv_buf[3]) { val += 2; zero_bits--; }
  if (recv_buf[4]) { val += 1; zero_bits--; }

  if ((zero_bits % 2) != recv_buf[5]) {
    fprintf(stderr, "PARITY CHECK FAILED! Connection "
    		    "desynchronized!\n");
    SigHandler(0);
    exit(1);
  }/* else printf("parity check: OK\n");*/

  printf("  val=%hi = %c\n", val, codes[val]);
  return codes[val];
}

/* Adds a new packet to the buffer */
void
NewPkt(short typ)
{
  static short cnt = 0;
  static short int recv_buf[6] = { '\0' };

  switch(typ) {
  case PCTYP_ARP:
    recv_buf[cnt] = PCTYP_ARP;
    break;
  case PCTYP_ICMP:
    recv_buf[cnt] = PCTYP_ICMP;
    break;
  default:
    err(1, "Error on NewPkt(): unknown typ value\n");
  }

  if (cnt == 5) {
    /* Received 6 Bits (a full character + parity bit) */
    char c;
  
    /*printf("INPUT=%hi%hi%hi%hi%hi-%hi\n", recv_buf[0], recv_buf[1],
    recv_buf[2], recv_buf[3], recv_buf[4], recv_buf[5]);*/
  
    c = ExtractCode(recv_buf);
    plaintext_buf[plaintext_buf_pos] = c;
    plaintext_buf_pos++;
    if (plaintext_buf_pos == BUFFERSIZ) {
      fprintf(stderr, "Error: Buffer full. Aborting.\n");
      /* Output buffer */
      SigHandler(0);
      /* Reset buffer */
      bzero(plaintext_buf, BUFFERSIZ);
      plaintext_buf_pos = 0;
    }
    bzero(recv_buf, sizeof(recv_buf));	
    cnt = -1;
  }
  fflush(stdout);
  cnt++;
}

void
SigHandler(int sig)
{
  printf("Received signal %i\n", sig);
  printf("Received Message: %s\n", plaintext_buf);
  /* Only exit on CTRL-C, not on sig==0 (full buffer+reset) */
  if (sig == SIGINT)
    exit(0);
}

int
main(int argc, char *argv[])
{
  pcap_t *descr;
  const u_char *packet;
  struct pcap_pkthdr hdr;
  struct ether_header *eptr;
  int datalink;

  if (argc != 2) {
    printf("usage: %s [device]\n", argv[0]);
    return 1;
  }

  if (signal(SIGINT, SigHandler) == SIG_ERR)
    err(1, "signal error");

  printf("RECEIVING MESSAGES - PRESS CTRL-C TO FINISH\n");

  if ((descr = SetIf(argv[1])) == NULL)
    return 1;

  datalink = pcap_datalink(descr);

  while (1) {
    if ((packet = pcap_next(descr, &hdr)) != NULL) {
      switch(datalink){
      case DLT_EN10MB:
        eptr = (struct ether_header *) packet;
        if (ntohs(eptr->ether_type) == ETHERTYPE_IP) {
          /* Must be ICMP since PCAP filter is set to
           * "arp or icmp[icmptype]=8" */
          NewPkt(PCTYP_ICMP);
        } else  if (ntohs(eptr->ether_type) == ETHERTYPE_ARP) {
          NewPkt(PCTYP_ARP);
        } else {
          /* Not part of Protocol Channel */
        }
        break;
      default:
        fprintf(stderr, "datalink type %i not supported.\n", datalink);
        exit(0);
      }
    }
    fflush(stdout);
  }
  return 0;
}

