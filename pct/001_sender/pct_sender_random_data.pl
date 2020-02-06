#!/usr/bin/perl

# pct_sender.pl, a sender tool for protocol channel messages (sends
# messages which can be received and interpreted by the tool
# 'pct_receiver.c'.
# (C) 2008 Steffen Wendzel, <steffen (at) wendzel (dot) de>.
#   http://www.wendzel.de
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

use strict;
# Dependencies: CPAN/Net::RAWIP und CPAN/Net::ARP
use Net::RawIP;
use Net::ARP;
use Socket;
use IO::Handle;
use Time::HiRes qw(usleep nanosleep);

STDOUT->autoflush(1);

# Send out ICMP packets from $srcip to $dstip with
# sequence number $seqnr
sub SendICMPPkt($srcip, $dstip, $seqnr)
{
  my $srcip = shift;
  my $dstip = shift;
  my $seqnr = shift;
  # ICMP-Payload like sent by Linux 2.6
  my $icmp_payload = "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a"
    . "\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29"
    . "\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35";

  my $icmppkt = Net::RawIP->new({
    ip  => {
      saddr => $srcip,
      daddr => $dstip
    },
    icmp => {
      type => 8,
      code => 0,
      id => $seqnr,
      sequence => 0,
      data => $icmp_payload
    },
  });
  
  $icmppkt->send;
}

sub SendUDPPkt($srcip, $dstip)
{
	my $srcip = shift;
	my $dstip = shift;
	
	my $dstip = (gethostbyname($dstip))[4];
	my $srcip = (gethostbyname($srcip))[4];
	
	my $zero_cksum = 0;
	
	my $src_port = 9999;
	my $dest_port = 9;
	my $len = 8;
	my $cksum = 0;
	my $data = "XXXX";
	my $udp_len = 12; #8+TEST
	my $udp_proto = 17; #17 is the code for udp, alternatively, you can getprotobyname. 
  
	my $ip_ver = 4;
	my $ip_len = 5;
	my $ip_ver_len = $ip_ver . $ip_len;
	my $ip_tos = 00;
	my ($ip_tot_len) = 12  + 20; # 12=8 (hdp hdr) + 4 byte payload + 20 byte ip header
	my $ip_frag_id = 19245;
	my $ip_frag_flag = "000";
	my $ip_frag_oset = "0000000000000";
	my $ip_fl_fr = $ip_frag_flag . $ip_frag_oset;
	my $ip_ttl = 64;


	my ($pkt) = pack('H2H2nnB16C2na4a4nnnna*',
		$ip_ver_len,$ip_tos,$ip_tot_len,$ip_frag_id,
		$ip_fl_fr,$ip_ttl,$udp_proto,$zero_cksum,$srcip,
		$dstip,$src_port,$dest_port,$len, $cksum, $data);

	socket(RAW, AF_INET, SOCK_RAW, 255) || die $!;
	setsockopt(RAW, 0, 1, 1);
	my ($destination) = pack('Sna4x8', AF_INET, $dest_port, $dstip);
	send(RAW,$pkt,0,$destination);

}



# Send out ARP reply packets
sub SendARPPkt
{
  my $dev = shift;
  my $srcip = shift;
  my $dstip = shift;
  my $srcmac = shift;
  my $dstmac = shift;
  
  Net::ARP::send_packet($dev, $srcip, $dstip, $srcmac, $dstmac, 'reply');
}

# Find out what bit combination is needed for the
# character $char and call the needed ARP/ICMP
# sending functions. Also send a parity bit here.
sub SendChar
{
  my $char = shift;
  my $seqnr = shift;
  my $dev = shift;
  my $srcip = shift;
  my $dstip = shift;
  my $srcmac = shift;
  my $dstmac = shift;
  my $bitrate = shift;
  
  my $lastproto = 1;


  while (1) {
  	if (rand(2)%2 == 1) {
	  	&SendICMPPkt($srcip, $dstip, 99);
#	  	print "ICMP...\n";
		print "I\n";
	  	$lastproto = 0;
	} else {
	  	&SendUDPPkt($srcip, $dstip);
#	  	print "UDP...\n";
		print "U\n";
	  	$lastproto = 1;
	}
#  	sleep(1/$bitrate);
	my $sleeptime;
	$sleeptime = ((1000000/$bitrate));
#	print "bitrate=",$bitrate,", sleeptime=",$sleeptime,"\n";
  	usleep($sleeptime); # in miliseconds (1sec = 10^6 microsec)
  }
  
  return $seqnr;
}

my $argc = $#ARGV + 1;

die "usage: [device] [src IP] [dest IP] [src MAC] [dest MAC] "
 . "[ICMP start seq] [bitrate (in bit/sec)] [payload]\n"
 . "example: pct_send eth0 192.168.2.21 192.168.2.1 "
 . "00:1d:09:35:87:c4 00:1d:09:35:87:c5 0x053c 1 \"Hello World\""
 unless $argc == 8;

my $dev = $ARGV[0];
my $srcip = $ARGV[1];
my $dstip = $ARGV[2];
my $srcmac = $ARGV[3];
my $dstmac = $ARGV[4];
my $seqnr = hex($ARGV[5]);
my $bitrate = $ARGV[6];
my $payload = uc ($ARGV[7]);
my $max_payload_len = 511;
my $i = 0;


if (length($payload) > $max_payload_len) {
  print "Payload too long. Max. " . $max_payload_len . " Bytes allowed!\n";
  exit 1;
}

&SendChar("a", $seqnr, $dev, $srcip, $dstip, $srcmac, $dstmac, $bitrate);

exit 0

