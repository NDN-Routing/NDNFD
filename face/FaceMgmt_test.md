# Manual Test for Face Management Protocol implementation

Face Management Protocol client `ndnfdc` and Face Management Protocol server in NDNFD, require manual testing.

These test cases only covers "addface" operation. "destroyface" is not fully covered.

## ccnd vs ndnfdc

### TCP without port number

	ccndstart
	ndnfdc add /ndn tcp e.hub.ndn.ucla.edu
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### TCP with port number

	ccndstart
	ndnfdc add /ndn tcp e.hub.ndn.ucla.edu 9695
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### UDP without port number

	ccndstart
	ndnfdc add /ndn udp e.hub.ndn.ucla.edu
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### UDP without port number

	ccndstart
	ndnfdc add /ndn udp e.hub.ndn.ucla.edu 9695
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

## NDNFD vs ccndc

### TCP

	sudo ndnfd
	ccndc add /ndn tcp e.hub.ndn.ucla.edu
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### UDP

	sudo ndnfd
	ccndc add /ndn udp e.hub.ndn.ucla.edu
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

## NDNFD vs ndnfdc

### TCP without port number

	sudo ndnfd
	ndnfdc add /ndn tcp e.hub.ndn.ucla.edu
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### TCP with port number

	sudo ndnfd
	ndnfdc add /ndn tcp e.hub.ndn.ucla.edu 9695
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### UDP without port number

	sudo ndnfd
	ndnfdc add /ndn udp e.hub.ndn.ucla.edu
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### UDP without port number

	sudo ndnfd
	ndnfdc add /ndn udp e.hub.ndn.ucla.edu 9695
	ccnping /ndn/arizona.edu

Expected: content from /ndn/arizona.edu

### UDP multicast

	sudo ndnfd
	ndnfdc add /ndn udp 224.0.23.170 59695

Expected: error

### Ethernet

	sudo ndnfd
	ndnfdc add /test7342 ether 08:00:27:57:32:74 eth1
	ccnping /test7342

Expected: on a separate console, `sudo tcpdump -i eth1 'ether dst 08:00:27:57:32:74'` should show packets

Note: the output of ccnping process is not significant

