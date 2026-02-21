# vstt (*very strange tunneling tool*)

(c) 2006-2025 by Steffen 'cdp_xe' Wendzel, [www.wendzel.de](https://www.wendzel.de).

### DISCLAIMER

This tool is for legal purposes only! Read the LICENSE file for license details.

I wrote this tool as a second semester undergraduate student in 2006. Expect no amazing code or tool.

### INTRODUCTION

vstt is a multi-protocol tunneling tool for some free unix-like operating systems. it can tunnel TCP connections through different network protocols, including ICMP and POP3.

### HOW TO COMPILE

Run the following command:

- Linux, *BSD, MacOS:  `make`
- Solaris: `make solaris`

### HOW TO INSTALL

Run `make install`.

### DOCUMENTATION

You can find the documentation in the *doc/* subdirectory (LaTeX and [PDF format](https://github.com/cdpxe/NetworkCovertChannels/blob/master/vstt/doc/doc.pdf)).

### Publications

- Our summary on covert channel hiding techniques:
  - S. Wendzel, S. Zander, B. Fechner, C. Herdin: [Pattern-based survey and categorization of network covert channel techniques](https://doi.org/10.1145/2684195), ACM Computing Survey (CSUR), Vol. 47(3), ACM, 2015.
- My comprehensive introduction to network covert channels:
  - my [university-level open online class on network information hiding](https://github.com/cdpxe/Network-Covert-Channels-A-University-level-Course).

Note: There is a [list of network covert channel tools](https://github.com/cdpxe/NetworkCovertChannels).
