# PHCCT – *Protocol Hopping Covert Channel Tool*

(C) 2007 Steffen Wendzel

This is a simple and the first implementation of a *protocol switching covert channel* (PHCC). A PHCC is a covert channel that switches the utilized network protocol from time to time, e.g. on a randomized basis (I invented this term while I was working for my diploma thesis at Kempten University of Applied Sciences, Germany, in 2007). The goal of using such a covert channel is that if one of the channels gets blocked, the others still remain. PHCCT simply applies different network protocols simultaneously for this purpose.

## Usage

Start the tool on two hosts and provide the peer address of the other host using the parameter `-a` to allow the both hosts to connect to each other:

`alice# ./phcct -a 192.168.2.100`

`bob# ./phcct -a 192.168.2.101`

To transfer data from `alice` to `bob`, connect locally to TCP port 9999 and send data to it (e.g. using `telnet` or `nc`).

## Notes

- A more sophisticated covert channel is one that actively probes for non-blocked network protocols, see my tool [NELphase](https://github.com/cdpxe/NELphase/).

## Publications

* Steffen Wendzel, Jörg Keller: *[Low-attention forwarding for mobile network covert channels](http://www.researchgate.net/profile/Steffen_Wendzel/publication/215661202_Low-attention_Forwarding_for_Mobile_Network_Covert_Channels/links/00b495349285e2ae43000000.pdf)*, 12th Conference on Communications and Multimedia Security (CMS 2011), LNCS vol. 7025, pp. 122-133, Springer, Ghent, Belgium, 2011.
- Steffen Wendzel: [Novel Approaches for Network Covert Storage Channels](https://www.researchgate.net/publication/236962097_Novel_Approaches_for_Network_Covert_Storage_Channels?ev=srch_pub&_sg=rhAuR9KbWdcA4AgO2E1dH6elFP74Vy6GHmVGBzfn%2BEQFywH%2F6Cv6pfzCuR4MhpSK_NQsZFaz4JSMzjjABbqoCxL1XYCV1rbrSl1gLqg2CSqBKm%2FjfEctgeaycWrJCR7Ej_Fj8dUZAMIkg5hu5ghQ5Ydl8tWU1fakPkMOsCtLNDu60GIpU2gYi4vWoyYSJAIVMR), PhD thesis, University of Hagen, submitted: Jan-2013, defended: May-2013.
- Wojciech Mazurczyk, Steffen Wendzel, Sebastian Zander, Amir Houmansadr, Krzysztof Szczypiorski: [Information Hiding in Communication Networks: Fundamentals, Mechanisms, and Applications](http://eu.wiley.com/WileyCDA/WileyTitle/productCd-1118861698.html), IEEE Series on Information and Communication Networks Security, Wiley-IEEE, 2016.

## Open Tasks

- add encryption
- add support for additional protocols
- add a packet mixing mode
- kernel based implementation
