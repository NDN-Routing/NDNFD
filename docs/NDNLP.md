# NDNLP support in NDNFD

NDNFD has partial support of [NDNLP](http://www.named-data.net/techreport/TR006-LinkProtocol.pdf). Fragmentation and reassembly feature is integrated, and used on Ethernet faces. Acknowledgement and retransmission feature is not supported, because packet loss is uncommon in local area network.

## NDNLP fragmentation on multicast

NDNLP sender operation is unchanged.

NDNLP receiver maintains a separate **partial message store** per sender address. Upon receiving a fragment, a partial message store is created for the sender if it does not exist, and the fragment is reassembled on that partial message store.  
Note: For the same sender address, unicast and each multicast group need separate partial message stores.

