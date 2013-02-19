# NDNFD Face Management Protocol

NDNFD is compatible with [CCNx Face Management and Registration Protocol](http://www.ccnx.org/releases/latest/doc/technical/Registration.html). The same protocol is used to create UDP and TCP faces, and register or unregister prefixes.

## Ethernet Face

NDNFD can work on Ethernet. CCNx Face Management Protocol is extended to represent Ethernet faces.

The same **FaceInstance** structure is used. These fields are redefined:

* **IPProto** = 97  
  This is the protocol number of Ethernet-within-IP Encapsulation, but we run on Ethernet directly without IP.
* **Host** = Ethernet MAC address of remote host or multicast group, in standard hex-digits-and-colons notation
* **MulticastInterface** = a series of "key=value" pairs, separated by newlines  

**MulticastInterface** field is extensible. An implementation SHOULD ignore keys it don't understand. Current defined keys are:

* **localif** = local interface name

The meaning of **Action**, **PublisherPublicKeyDigest**, **FaceID**, and **FreshnessSeconds** are unchanged. **Port** and **MulticastTTL** fields are not relevant to Ethernet, and MUST NOT be present. 

An example of a newface request is:

	<FaceInstance>
		<Action>newface</Action>
		<IPProto>97</IPProto>
		<Host>08:00:27:01:01:01</Host>
		<MulticastInterface>localif=eth1</MulticastInterface>
	</FaceInstance>

