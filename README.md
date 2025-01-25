# [Steffen Wendzel](https://www.wendzel.de)'s Network Covert Channel Tools

This repository contains some of my covert channel projects and also refers to some other repositories where I host my additional covert channel projects.

If you need an introduction, check out **[my free open online class on network covert channels](https://github.com/cdpxe/Network-Covert-Channels-A-University-level-Course/)**.

## In this repository:

- [pct](https://github.com/cdpxe/NetworkCovertChannels/tree/master/pct): *Protocol Channel Tool* (`pct` a PoC implementation to show that so-called protocol channels are feasible).
- [phcct](https://github.com/cdpxe/NetworkCovertChannels/tree/master/phcct): *Protocol Hopping Covert Channel Tool* (`phcct` was the first (2007) implementation of a protocol hopping covert channel).
- [vstt](https://github.com/cdpxe/NetworkCovertChannels/tree/master/vstt): *Very Strange Tunneling Tool* (This was my first network CC tool. I wrote `vstt` as a 2nd semester undergrad student. It can tunnel through ICMP, TCP, ...).
- [pcaw](https://github.com/cdpxe/NetworkCovertChannels/tree/master/pcaw): *Protocol Channel-aware Active Warden* (`pcaw` is a countermeasure to reduce the error-free transmission performance of so-caled [protocol switching covert channels](https://www.computer.org/csdl/proceedings-article/lcn/2012/06423628/12OmNBC8AyY)*/[pct](https://github.com/cdpxe/NetworkCovertChannels/tree/master/pct) (and *[protocol hopping covert channels](https://github.com/cdpxe/NetworkCovertChannels/tree/master/phcct)*). I wrote this in 2012 while working on my PhD thesis).

## My other covert channel projects on GitHub

- [CCEAP](https://github.com/cdpxe/CCEAP): *Covert Channel Educational Analysis Protocol* (a tool for teaching network covert channel patterns).
- [NELphase](https://github.com/cdpxe/NELphase): *Network Environment Learning Phase* (a tool that implements a covert channel capable of performing an network environment learning (NEL) phase and that can be used to test active and passive wardens).

## Tools of my students

- [DYST](https://github.com/NIoSaT/DYST): The first *history covert channel* implementation
- [WiFi Reconnection-based Covert Channel](https://github.com/NIoSaT/WiFi_Reconnection_CovertChannel): My PhD student Sebastian Zillien developed this PoC code to demonstrate a WiFi reconnection-based covert channel that exploits pattern [PT15 (Artificial Reconnections)](https://ih-patterns.blogspot.com/p/pt15-artificial-reconnections.html) by forcing WiFi clients to reconnect. The channel can provide anonymity for covert sender and covert receiver.
- [CoAP Reset-/Reconnection-based Covert Channels](https://github.com/NIoSaT/CoAP-Covert-Channels): Another tool by my PhD students.

## Other Stego Tools

- Luca's list of stego tools on Github: [https://github.com/lucacav/steg-tools](https://github.com/lucacav/steg-tools)

