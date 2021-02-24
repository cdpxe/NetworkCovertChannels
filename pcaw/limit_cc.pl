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

my $last_proto = -1;
my $proto_changed = 0;

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
			if ($last_proto != $hexbyte and $last_proto != -1) {
#				print "changed protocol SRC/DST x/y ... delaying this pkt.";
				$proto_changed = 1;
			} else {
				$proto_changed = 0;
			}
			$last_proto = $hexbyte;
		}
	}
#	print "\n";

	# process the received message
	my $now = gettimeofday;
	
	# Figure out when it needs to go back out...
	my $alarm = $now;
	if ($proto_changed == 1) {
		my $applied_delay = (10*$delay);
		print "APPLIED_DELAY=",$applied_delay,"\n";
		$alarm += $applied_delay;
	}
	# Queue if for the future
	push @queue, { time => $alarm, msg => $msg };
	
	if ($last_proto == "11") {
		print $alarm, " U\n";
	} else {
		print $alarm, " I\n";
	}
	
	$counter++;
	if ($counter == 100) {
		#exit(1);
	}
    }
}

