#!/bin/bash


if [ "$1" = "" ]; then
	echo "please provide bitrate"
	exit 1
fi

dev=eth1
bitrate=$1
srcip=192.168.3.1
dstip=192.168.3.2
srcmac=08:00:27:bf:3b:8e
dstmac=08:00:27:2f:7a:0a
icmpseq=17

echo "using dev=$dev bitrate=$bitrate pkts/sec src=$srcip dst=$dstip src-mac=$srcmac dst-mac=$dstmac icmpseq=$icmpseq"

sudo ./pct_sender.pl $dev $srcip $dstip $srcmac $dstmac $icmpseq $bitrate Hello

#NOTREACHED

