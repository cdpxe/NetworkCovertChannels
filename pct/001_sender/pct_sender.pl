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
  my %codes = (
  'A' => "00000", 'B' => "00001", 'C' => "00010", 'D' => "00011",
  'E' => "00100", 'F' => "00101", 'G' => "00110", 'H' => "00111",
  'I' => "01000", 'J' => "01001", 'K' => "01010", 'L' => "01011",
  'M' => "01100", 'N' => "01101", 'O' => "01110", 'P' => "01111",
  'Q' => "10000", 'R' => "10001", 'S' => "10010", 'T' => "10011",
  'U' => "10100", 'V' => "10101", 'W' => "10110", 'X' => "10111",
  'Y' => "11000", 'Z' => "11001", ' ' => "11010", '_' => "11011",
  '-' => "11100", '$' => "11101", '.' => "11110", ',' => "11111");
  my $i = 0;
  my $bits = $codes{$char};
  my $zero_bits = 0;

  print "sending=" . $bits . "\n";

  while ($i < length($bits)) {
    $char = substr($bits, $i, 1);
    print "sending bit " . $i . "=" . $char . " ";
    if ($char eq "0") {
      print "ARP";
      &SendARPPkt($dev, $srcip, $dstip, $srcmac, $dstmac);
      $zero_bits = $zero_bits + 1;
    } elsif ($char eq "1") {
      print "ICMP";
      &SendICMPPkt($srcip, $dstip, $seqnr);
      $seqnr = $seqnr + 1;
    } else {
      print "Illegal bit value!";
      exit(1);
    }
    print "\n";
    $i++;
  }
  
  # send parity bit
  if (($zero_bits % 2) == 1) {
    # send 1
    print "Sending parity 1\n";
    &SendICMPPkt($srcip, $dstip, $seqnr);
    $seqnr = $seqnr + 1;
  } else {
    # send 0
    print "Sending parity 0\n";
    &SendARPPkt($dev, $srcip, $dstip, $srcmac, $dstmac);
  }
  
  return $seqnr;
}

my $argc = $#ARGV + 1;

die "usage: [device] [src IP] [dest IP] [src MAC] [dest MAC] "
 . "[ICMP start seq] [payload]\n"
 . "example: pct_send eth0 192.168.2.21 192.168.2.1 "
 . "00:1d:09:35:87:c4 00:1d:09:35:87:c5 0x053c \"Hello World\""
 unless $argc == 7;

my $dev = $ARGV[0];
my $srcip = $ARGV[1];
my $dstip = $ARGV[2];
my $srcmac = $ARGV[3];
my $dstmac = $ARGV[4];
my $seqnr = hex($ARGV[5]);
my $payload = uc ($ARGV[6]);
my $max_payload_len = 511;
my $i = 0;

if (length($payload) > $max_payload_len) {
  print "Payload too long. Max. " . $max_payload_len . " Bytes allowed!\n";
  exit 1;
}

while ($i < length($payload)) {
  my $char = substr($payload, $i, 1);
  print "sending payload[" . $i . "]=" . $char . "\n";
  $seqnr = &SendChar($char, $seqnr, $dev, $srcip, $dstip, $srcmac, $dstmac);
  print "Seqnr now=".$seqnr."\n";
  $i++;
}

exit 0

