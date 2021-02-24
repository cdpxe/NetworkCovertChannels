#!/bin/bash

echo -n "Loading modules ... "
echo -n "ip_queue"; modprobe ip_queue
echo -n ", iptable_filter"; modprobe iptable_filter
echo -n ", ipt_ttl"; modprobe ipt_ttl
echo

echo -n "Setting up iptables rule ... "
#iptables -A INPUT --source 192.168.3.1 -j QUEUE
iptables -A INPUT -i eth1 -j QUEUE
echo "done."

exit 0


