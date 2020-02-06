# PHCCT – *Protocol Hopping Covert Channel Tool*

This is a simple (and the first) implementation of a *protocol switching covert channel* (PHCC). It was written in 2007. Publications and more information on this type of covert channel can be found [on my website](https://steffen-wendzel.blogspot.com/p/covert-channel-software.html#phcct).

## Usage

Start the tool on two hosts and provide the peer address of the other host using the parameter `-a` to allow the both hosts to connect to each other:

`alice# ./phcct -a 192.168.2.100`

`bob# ./phcct -a 192.168.2.101`

To transfer data from `alice` to `bob`, connect locally to TCP port 9999 and send data to it (e.g. using `telnet` or `nc`).


