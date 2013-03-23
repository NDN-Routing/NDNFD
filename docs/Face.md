# Face System

NDNFD face system consists of:

* abstract face
* wire protocol
* abstraction of datagram and stream faces
* lower layer
* face management

## Abstract Face

ndnfd::Face class represents a **Face**, which is a logical connection to a local entity (the internal client, or a local application), or one or more remote peers. A Face can be used send or receive messages; a **Message** could be either an Interest or a ContentObject. Some faces can also be used to accept incoming connections as new faces; this is mainly used for stream listeners.

Each face has a **FaceId**. The FaceId is assigned by the **face manager**. In the lifetime of a NDNFD process, each FaceId uniquely refers to a face, and is never reused later.

Each face has a **FaceKind** that indicate who it's talking to. It does not change during the lifetime of the face. Its values are:

* Internal: talk to the internal client
* App: talk to a local application
* Multicast: talk to multiple remote peers
* Unicast: talk to one remote peer

Each face has a **FaceStatus** that indicate the current status. It can change at any time. Its values are:

* Connecting: Outgoing stream connection is in progress.
* Undecided: Incoming connection is established, but no message has been received.
* Established: Connection is fully working.
* Closing: The face will be closed after the last queued message is sent.
* Closed: Connection is closed. The face is no longer usable, and will be deleted immediately.
* ConnectError: Outgoing stream connection fails. The face is no longer usable, and will be deleted immediately.
* ProtocolError: Wire protocol reports a protocol error when decoding incoming packets on a stream face. The face is no longer usable, and will be deleted immediately.
* Disconnect: The stream connection is closed by the peer. The face is no longer usable, and will be deleted immediately.

## Wire Protocol

A **WireProtocol** (subclass of ndnfd::WireProtocol) provides a **wire format**: how messages (Interests or ContentObjects) are encoded as packets, and how packets are decoded as messages.

A WireProtocol have the freedom to:

* decide how a message is represented on the wire (using CCNB, or some other format)
* fragment a message into multiple packets
* combine multiple messages into one packet
* encrypt messages / packets
* ...

Multiple WireProtocols can be chained/combined to create a WireProtocol that can recognize multiple packet formats, etc.

A WireProtocol is either stateless or stateful. A stateful WireProtocol stores its state in a subclass of ndnfd::WireProtocolState; the state is per face, per remote peer. The state object is stored by other components (StreamFace or DgramChannel), and is presented to the WireProtocol on each Encode and Decode call; this allows the state object to be deleted when face / peer is closed.

WireProtocol has two important methods:

* Encode is called to encode a message into zero or more packets.
* Decode is called to decode a packet into zero or more messages.

### CCNB

ndnfd::CcnbWireProtocol is the simplest wire protocol. Current NDNFD uses ccnd.c as the core of router, which requires all message to be CCNB-encoded (ndfd::CcnbMessage).

CCNB wire protocol is stateless in datagram mode. Messages and packets have one-to-one relationship, but they must be verified for integrity on conversion.

CCNB wire protocol is stateful in stream mode. Messages are read from incoming buffers by ccn\_skeleton\_decode, and incomplete messages are kept in the state.

### NDNLP

ndnfd::NdnlpWireProtocol implements fragmentation feature of NDN Link Protocol (NDNLP). It is stateful, and works with datagram faces only. To fragment a message into multiple packets, Encode returns multiple packets. To reassemble multiple packets as one message, Decode returns zero message until the last packet is received.

NdnlpWireProtocol is chained with CcnbWireProtocol to encode message before fragmentation, and to decode message after reassembly.

## Abstraction of Datagram and Stream Faces

### Datagram Face

ndnfd::DgramFace and ndnfd::DgramChannel classes are abstraction of datagram-based faces, such as UDP and Ethernet.

**DgramChannel** plays the major role. **DgramFace**s are slaves of DgramChannel: they Send and Receive by calling into DgramChannel. 

The datagram socket, after bind() but without connect(), is owned by DgramChannel. This socket is registered with PollManager for POLLIN event, so that DgramChannel is notified on packet arrival.

#### unicast operation

DgramChannel maintains a peer table. Each peer entry contains peer address, unicast face, and wire protocol state; the latter two fields can be null. There is also a **fallback face**, which will receive messages from peers without a unicast face, but the fallback face cannot be used to send messages.

Upon receiving a packet, DgramChannel retrieves (or creates) the peer entry according to sender address of the packet, and the WireProtocol is called to decode the packet. If one or more messages are returned, they are delivered to the unicast face in the peer entry, or the fallback face.

To send a message, DgramFace hands over the message to DgramChannel. DgramChannel retrieves the peer entry according to the peer address of the DgramFace, and the WireProtocol is called to encode the message. If one or more packets are returned, they are sent via the datagram socket to the peer address.

#### multicast operation

Due to the difference in multicast API, DgramChannel does not support multicast on its own. Nevertheless, DgramChannel contains structures that helps with multicast implementation.

Each multicast group joined by the datagram socket has a **multicast entry**, which contains group address, multicast face, wire protocol state for outgoing messages, per-peer wire protocol state for incoming packet.

Upon receiving a packet on a group, DgramChannel retrieves (or does not create) the multicast entry of that group and the per-peer wire protocol state according to sender address of the packet, and the WireProtocol is called to decode the packet. If one or more messages are returned, they are delivered to the multicast face.

To send a message, DgramFace hands over the message to DgramChannel. DgramChannel retrieves the multicast entry according to the peer address of the DgramFace (which is actually the group address, when FaceKind is Multicast) and the outgoing wire protocol state, and the WireProtocol is called to encode the message. If one or more packets are returned, they are sent via the datagram socket to the group address.

### Stream Face

ndnfd::StreamFace class is abstraction of stream-based faces, such as TCP and UNIX.

The stream socket, after connect() or accept(), is owned by StreamFace. This socket is registered with PollManager for POLLIN event, so that StreamFace is notified on data arrival.

Upon receiving a buffer of octets, the WireProtocol is called to decode these octets. If one or more messages are returned, they are delivered as received messages.

To send a message, the WireProtocol is called to encode the message. If one or more buffers are returned, they are written to the stream socket in order. In case the socket is blocked for writing, the octets are kept in a queue, and StreamFace registers the socket with PollManager for POLLOUT event; when the socket becomes available for writing again, queued octets are written before any new buffer, and POLLOUT is cleared. Whenever the queue is not empty, the router is requested to slow down on this face (this is currently implemented as setting CCN\_FACE\_NOSEND flag).

### Stream Listener

ndnfd::StreamListener class is abstraction of stream listener faces, such as TCP.

The stream socket, after listen(), is owned by StreamListener. This socket is registered with PollManager for POLLIN event, so that StreamListener is notified on connection request.

Upon arrival of a connection request, it is accepted, and a new StreamFace is created for the accepted socket.

## Lower Layer

### UDP and TCP

ndnfd::UdpFaceFactory creates DgramChannel for UDP. ndnfd::TcpFaceFactory creates StreamFace for TCP outgoing connection, and StreamListener to listen for TCP incoming connection. Both supports IPv4 and IPv6, and uses sockaddr\_in or sockaddr\_in6 for address; IPv4-mapped-IPv6 addresses are not supported.

ndnfd::UdpSingleMcastChannel is a subclass of DgramChannel that allows communicating on a single multicast group. Each (local IP, local port) can join one UDP multicast group, and every packet received on this port is assumed to be addressed to the multicast group. Joining multiple multicast groups is not supported.

### Ethernet

ndnfd::EtherFaceFactory creates DgramChannel (PcapChannel) for Ethernet. Address type is ether\_address. Wire protocol is fixed to NDNLP, and Ethernet MTU is honored.

ndnfd::PcapChannel is a subclass of DgramChannel that implements Ethernet communication using libpcap. This works on both linux (via AF\_PACKET socket) and BSD (via Berkeley Packet Filter). Ethernet multicast is supported, but the group address is restricted to 01:00:5E:00:00:00 - 01:00:5E:7F:FF:FF, and joining Ethernet multicast group is implemented as joining an IPv4 multicast group that is mapped to this Ethernet address.

## Face Management

The **face manager** (ndnfd::FaceMgr class) maintains a table of active faces indexed by FaceId.

ndnfd::FaceMgr::StartDefaultListeners method creates default UDP/Ethernet DgramChannels and UNIX/TCP StreamListeners, and join default UDP/Ethernet multicast groups.

ndnfd::FaceMgr::FaceMgmtReq answers a CCNx Face Management protocol command to create or destroy additional UDP/TCP/Ethernet faces.

