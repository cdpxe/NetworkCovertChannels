#!/bin/bash

echo
echo "I hope you ran setup.sh first! (Press CTRL+C if not)"
read nothing
echo
echo -n "Setting up iptables rule ... "
#iptables -A INPUT --source 192.168.3.1 -j QUEUE
iptables -A INPUT -i eth0 -j QUEUE
echo "done."

exit 0


