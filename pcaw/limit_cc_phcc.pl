#!/usr/bin/perl -w
#
# limit_cc.pl Nov-05-2011:
# delay packets probably belonging to protocol channels (PC) as well as
# protocol hopping covert channels (PHCC). We do only focus on the TCP/IP
# Internet layer here and thus only evaluate the content of the IPv4 header's
# 'protocol' field.
#
# (c) Copyright 2011-2012, Steffen Wendzel (http://www.wendzel.de),
# based on: delay-net.pl by Daniel Berrange:
#     (c) Copyright 2005, Daniel Berrange, based on:
#     countertrace, a traceroute faker
#     (c) Copyright  2002 Michael C. Toren <mct@toren.net>
# Released under the terms of the GNU General Public Liense, version 2.

use strict;
use warnings;
use IPTables::IPv4::IPQueue qw(:constants);
use Digest::MD5 qw(md5_base64);
use Time::HiRes qw(gettimeofday);
use Term::ANSIColor;

my @queue;
my $delay = shift @ARGV;
$delay = 250 unless defined $delay;
my $id = 0;
my %seen;
$delay = $delay / 1000;
my $counter;
$counter=0;

print "Delay by ", (10*$delay),  " seconds\n";

$| = 1;

my $ipq = new IPTables::IPv4::IPQueue
		(copy_mode => IPQ_COPY_PACKET, copy_range => 2000)
	or die "Could not initialize IPQ: ", IPTables::IPv4::IPQueue->errstr, "\n";

my $last_port = -1;
my $dstport = -1;
my $port_changed = 0;

&process();
exit 0;


sub process {
    while (1) {
	# process oustanding queued packets
	my $now1 = gettimeofday;
	@queue = grep {
	    if ($now1 + 0.003 >= $_->{time}) {
		#print "Deliver: ", $_->{msg}->packet_id, "\n";
		$ipq->set_verdict($_->{msg}->packet_id, NF_ACCEPT);
		0;
	    } else {
		1;
	    }
	} @queue;
	
	# set the timeout for when the next packet is due to go off, or 60 seconds
	my ($nap) = (sort { $a->{time} <=> $b->{time} } @queue, { time => $now1 + 60 });
	$nap = $nap->{time} - gettimeofday;
	
	# Get next message off the queue, timing out after $nap u-seconds
	my $msg = $ipq->get_message($nap * 1_000_000);

	unless (defined $msg) {
	    die "ipq: [", IPTables::IPv4::IPQueue->errstr, "]\n"
		unless (IPTables::IPv4::IPQueue->errstr =~ /^timeout/i);
	    next;
	}
	unless ($msg->data_len) {
	    # Ignoring zero-length ipq message
	    next;
	}

	#print "hw_protocol: ", $msg->hw_protocol, " (type ", $msg->hw_type, ") ";
	
	# parse the payload
	my $payload = $msg->payload;
	my $len = length($payload);
	for (my $c=0; $c < $len; $c++) {
		my $byte = substr($payload, $c, 1);
		my $asciibyte = ord($byte);
		my $hexbyte = sprintf("%02x", $asciibyte);
		
		if ($c==9) {
#			print "IP protocol: 0x$hexbyte ";
			# This time (for PHCCT), we need to determine, that we deal with TCP
			# since all protocols utilized by PHCCT are TCP protocols. Afterwards,
			# we can identify them using the TCP dest. port information.
			if ($hexbyte==6) {
				#print "TCP found.\n";
				# determine dst. port
				# offset: 20 byte IP header + first 2 byte of TCP
				# header = 22 bytes
				# dst. port has size of 2 bytes.
				$dstport = substr($payload, 22, 2);
				
				my $dstportA = substr($payload, 22, 1);
				my $dstportB = substr($payload, 23, 1);
				
				my $asciidstportA = ord($dstportA);
				my $asciidstportB = ord($dstportB);
				my $hexdstportAB = sprintf("%02x%02x", $asciidstportA, $asciidstportB);
				#my $hexdstport = unpack("N", pack("B16", $dstport));
				print "====> ",$hexdstportAB,"\n";
				#print "=====> ",$hexdstport,"\n";
				$dstport = $hexdstportAB;
			}
			
			if ($last_port !~ $dstport and $last_port != -1) {
				#print "changed protocol SRC/DST x/y ... delaying this pkt.";
				$port_changed = 1;
			} else {
				$port_changed = 0;
			}
			$last_port = $dstport;
		}
	}
#	print "\n";

	# process the received message
	my $now = gettimeofday;
	
	# Figure out when it needs to go back out...
	my $alarm = $now;
	if ($port_changed == 1) {
		my $applied_delay = (10*$delay);
		print "APPLIED_DELAY=",$applied_delay,"\n";
		$alarm += $applied_delay;
	}
	# Queue if for the future
	push @queue, { time => $alarm, msg => $msg };
	
	#if ($last_port == "P") {
		#print $alarm, " ", hex($last_port), " \n";
		#printf "=> %d %#x %#bn\n" , ($last_port) x3;
	#} else {
	#	print $alarm, " I\n";
	#}
	
	$counter++;
	if ($counter == 100) {
		#exit(1);
	}
    }
}

