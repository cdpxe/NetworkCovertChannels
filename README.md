# [Steffen Wendzel](https://www.wendzel.de)'s Network Covert Channel Tools

This repository contains some of my covert channel projects and also refers to some other repositories where I host my additional covert channel projects.

If you need an introduction, check out **[my free open online class on network covert channels](https://github.com/cdpxe/Network-Covert-Channels-A-University-level-Course/)**.

## In this repository:

- [pcaw](https://github.com/cdpxe/NetworkCovertChannels/tree/master/pcaw), 2012 (published: ICIMP 2021): *Protocol Channel-aware Active Warden* (`pcaw` is a countermeasure to reduce the error-free transmission performance of so-caled *[protocol switching covert channels](https://www.computer.org/csdl/proceedings-article/lcn/2012/06423628/12OmNBC8AyY)*/[pct](https://github.com/cdpxe/NetworkCovertChannels/tree/master/pct) (and *[protocol hopping covert channels](https://github.com/cdpxe/NetworkCovertChannels/tree/master/phcct)*). I wrote this in 2012 while working on my PhD thesis).
- [pct](https://github.com/cdpxe/NetworkCovertChannels/tree/master/pct), 2008: *Protocol Channel Tool* (`pct` a PoC implementation to show that so-called protocol channels/protocol switching covert channels are feasible).
- [phcct](https://github.com/cdpxe/NetworkCovertChannels/tree/master/phcct), 2007: *Protocol Hopping Covert Channel Tool* (`phcct` was the first (2007) implementation of a protocol hopping covert channel).
- [vstt](https://github.com/cdpxe/NetworkCovertChannels/tree/master/vstt), 2006: *Very Strange Tunneling Tool* (This was my first network CC tool. I wrote `vstt` as a 2nd semester undergrad student. It can tunnel through ICMP, TCP, ...).

## My other covert channel projects on GitHub

- N.N., 2026 (published: IFIP SEC 2026), a compression algorithm; will be made public soon.
- [NELphase](https://github.com/cdpxe/NELphase), 2017: *Network Environment Learning Phase* (a tool that implements a covert channel capable of performing an network environment learning (NEL) phase and that can be used to test active and passive wardens).
- [CCEAP](https://github.com/cdpxe/CCEAP), 2016 (published: ACM CCS'16): *Covert Channel Educational Analysis Protocol* (a tool for teaching network covert channel patterns).

## Tools published together with my students

- [DYST](https://github.com/NIoSaT/DYST), 2022-2025 (published: IEEE TDSC, 2025): This is an implementation of a so-called history covert channel that allows *covert channel amplification*. The code was implemented by two of my PhD students for one of our TDSC papers.
- [OPPRESSION](https://github.com/Stego-Punk-Lab/OPPRESSION), 2024 (published: ACM AsiaCCS 2024): An implementation of a history covert channel that is based on online text repositories.
- [WiFi Reconnection-based Covert Channel](https://github.com/NIoSaT/WiFi_Reconnection_CovertChannel), 2021 (published: IFIP SEC 2021): My PhD student Sebastian Zillien developed this PoC code to demonstrate a WiFi reconnection-based covert channel that exploits pattern [PT15 (Artificial Reconnections)](https://patterns.omi.uni-ulm.de/NIHPattern/) by forcing WiFi clients to reconnect. The channel can provide anonymity for covert sender and covert receiver.
- [CoAP Reset-/Reconnection-based Covert Channels](https://github.com/NIoSaT/CoAP-Covert-Channels), 2021 (published: EICC 2021): Another tool by my PhD students.

## Other Stego Tools

- Luca's list of stego tools on Github: [https://github.com/lucacav/steg-tools](https://github.com/lucacav/steg-tools)
