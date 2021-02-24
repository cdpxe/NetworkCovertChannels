# PCAW - Protocol Channel-aware Active Warden

`pcaw` limits the efficiency of [protocol channels](https://github.com/cdpxe/NetworkCovertChannels/tree/master/pct). In other words, it is an active warden.

### Quick Start

Essentially, one needs to (adjust and) execute the `setup.sh` script and then run one of the limitation scripts:

- `limit_cc.pl` – limitation of protocol switching covert channels (=protocol channels), i.e. covert channels that transfer secret information solely through the utilized protocol of succeeding network packets.
- `limit_cc_randomized.pl` – same as `limit_cc.pl` but uses a randomized delay for better efficiency.
- `limit_cc_phcc.pl` – specific PCAW version for *protocol hopping covert channels*, i.e. covert channels that embed secret information *inside the content* of network packets, *but utilize several different network protocols* in a succeeding manner.
- `limit_cc_bacnet.pl` – specific PCAW version for the BACnet protocol's message ID field. Requires execution of `setup_bacnet.sh`.

### Publications

- Steffen Wendzel, Jörg Keller:
  [Design and Implementation of an Active Warden Addressing Protocol Switching Covert Channels](https://www.researchgate.net/publication/229092168_Design_and_Implementation_of_an_Active_Warden_Addressing_Protocol_Switching_Covert_Channels?ev=srch_pub&_sg=jqQGRsDWfwRzu7RhfH0qyeQvrkpCMNVQeMWb1Tz0vz%2BwbwR5ci7IpZU3suKveg12_4b3SF39cRIxo%2FvLaewvDaviWEZwCs%2FhBWDwockrx9%2FrRu4fpDCTmQTM%2B4jiEJuCS_HXFbseG2qpv10xzxYF88%2FUeD4P07GXAgZpGiIZQlajy%2BI5DZKAj7zjNyDHKR2UT4),
  7th International Conference on Internet Monitoring and Protection (ICIMP 2012), pp. 1-6, Stuttgart, Germany, 2012.

- Steffen Wendzel, Sebastian Zander:
  [Detecting Protocol Switching Covert Channels](http://dx.doi.org/10.1109/LCN.2012.6423628),
  37th IEEE Conf. on Local Computer Networks (LCN), pp. 280-283, Clearwater, Florida, 2012. (download via IEEE Xplore)

- Steffen Wendzel, Jörg Keller:
  [Preventing Protocol Switching Covert Channels](https://www.researchgate.net/publication/233765874_Preventing_Protocol_Switching_Covert_Channels),
  International Journal On Advances in Security, vol. 5 no. 3&4, pp. 81-93, 2012.

- Steffen Wendzel:
  [Novel Approaches for Network Covert Storage Channels](https://www.researchgate.net/publication/236962097_Novel_Approaches_for_Network_Covert_Storage_Channels?ev=srch_pub&_sg=rhAuR9KbWdcA4AgO2E1dH6elFP74Vy6GHmVGBzfn%2BEQFywH%2F6Cv6pfzCuR4MhpSK_NQsZFaz4JSMzjjABbqoCxL1XYCV1rbrSl1gLqg2CSqBKm%2FjfEctgeaycWrJCR7Ej_Fj8dUZAMIkg5hu5ghQ5Ydl8tWU1fakPkMOsCtLNDu60GIpU2gYi4vWoyYSJAIVMR),
  PhD thesis, University of Hagen, submitted: Jan-2013, defended: May-2013.

