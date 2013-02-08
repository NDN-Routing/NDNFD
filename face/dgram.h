#ifndef NDNFD_FACE_DGRAM_H_
#define NDNFD_FACE_DGRAM_H_
#include "face/face.h"
#include "face/wireproto.h"
#include "face/addrverifier.h"
#include "core/pollmgr.h"
namespace ndnfd {

class DgramChannel;

// A DgramFace is a Face that communicates on a shared DgramChannel.
class DgramFace : public Face {
 public:
  DgramFace(Ptr<DgramChannel> channel, const NetworkAddress& peer);
  const NetwordAddress& peer(void) const { return this->peer_; }
  Ptr<DgramChannel> channel(void) const { return this->channel_; }
  
  virtual bool CanSend(void) const { return true; }
  virtual bool CanReceive(void) const { return true; }
  
  virtual void Send(Ptr<Message> message);
  
 private:
  NetwordAddress peer_;
  Ptr<DgramChannel> channel_;
  DISALLOW_COPY_AND_ASSIGN(DgramFace);
};

// A DgramFallbackFace is a DgramFace that receives messages
// from peers with no DgramFace.
// It cannot be used for sending.
class DgramFallbackFace : public DgramFace {
 public:
  DgramFallbackFace(Ptr<DgramChannel> channel);
  
  virtual bool CanSend(void) const { return false; }
  
 private:
  DISALLOW_COPY_AND_ASSIGN(DgramFallbackFace);
};

// A DgramChannel represents a datagram socket shared by zero or more DgramFaces.
class DgramChannel : public Element, public IPollClient {
 public:
  // fd: fd of the socket, after bind(local_addr)
  DgramChannel(int fd, Ptr<IAddressVerifier> av, Ptr<WireProtocol> wp);
  
  // GetFallbackFace returns the fallback face: "unsolicited" messages
  // (from peers without a DgramFace) are received on this face.
  virtual Ptr<DgramFallbackFace> GetFallbackFace(void) const { return this->fallback_face_; }
  
  // GetFace returns a unicast face for a peer.
  // One is created if it does not exist.
  virtual Ptr<DgramFace> GetFace(NetworkAddress peer);
  
  // FaceSend sends message to face->peer() over the channel.
  // This is called by DgramFace.
  virtual void FaceSend(Ptr<DgramFace> face, Ptr<Message> message);
  
  // PollCallback is invoked with POLLIN when there are packets
  // on the socket to read.
  virtual void PollCallback(int fd, short revents);
  
 protected:
  // A PeerEntry represents per-peer information.
  // If .wp()->IsStateful() is false, Ptr<WireProtocolState> is NULL.
  typedef std::tuple<Ptr<DgramFace>,Ptr<WireProtocolState>> PeerEntry;

  int fd(void) const { return this->fd_; }
  Ptr<IAddressVerifier> av(void) const { return this->av_; }
  Ptr<WireProtocol> wp(void) const { return this->wp_; }
  std::unordered_map<NetworkAddress,PeerEntry>& peers(void) { return this->peers_; }
  
  // ReceiveFrom calls recvfrom() syscall to read one or more packets
  // from the socket, and call DeliverPacket for each packet.
  virtual void ReceiveFrom(void);
  
  // DeliverPacket decodes pkt, delivers any result messages
  // to the Face associated with peer or the fallback face.
  virtual void DeliverPacket(const NetworkAddress& peer, Ptr<Buffer> pkt);
  
 private:
  int fd_;
  Ptr<IAddressVerifier> av_;
  Ptr<WireProtocol> wp_;
  std::unordered_map<NetworkAddress,PeerEntry> peers_;
  Ptr<DgramFallbackFace> fallback_face_;
  
  DISALLOW_COPY_AND_ASSIGN(DgramChannel);
};

// A McastFace sends and receives messages on a multicast group.
class McastFace : public Face, public IPollClient {
 public:
  // fd_recv: fd of the receiving socket, after bind(any) and joining multicast group
  // fd_send: fd of the sending socket, after bind(local_addr)
  McastFace(int fd_recv, int fd_send, const NetworkAddress& mcast_group, Ptr<IAddressVerifier> av, Ptr<WireProtocol> wp);

  virtual bool CanSend(void) const { return true; }
  virtual bool CanReceive(void) const { return true; }

  virtual void Send(Ptr<Message> message);
  
  // PollCallback is invoked with POLLIN when there are packets
  // on the receiving socket to read.
  virtual void PollCallback(int fd, short revents);
  
 protected:
  int fd_recv(void) const { return this->fd_recv_; }
  int fd_send(void) const { return this->fd_send_; }
  const NetworkAddress& mcast_group(void) const { return this->mcast_group_; }
  Ptr<IAddressVerifier> av(void) const { return this->av_; }
  Ptr<WireProtocol> wp(void) const { return this->wp_; }
  std::unordered_map<NetworkAddress,Ptr<WireProtocolState>>& peers(void) { return this->peers_; }
  
 private:
  int fd_recv_;
  int fd_send_;
  NetworkAddress mcast_group_;
  Ptr<IAddressVerifier> av_;
  Ptr<WireProtocol> wp_;
  std::unordered_map<NetworkAddress,Ptr<WireProtocolState>> peers_;
  Ptr<DgramFallbackFace> fallback_face_;
  
  DISALLOW_COPY_AND_ASSIGN(McastFace);
};

};//namespace ndnfd
#endif//NDNFD_FACE_DGRAM_H
