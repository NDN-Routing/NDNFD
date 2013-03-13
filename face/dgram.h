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
  virtual void Init(void);
  virtual ~DgramFace(void) {}

  const NetworkAddress& peer(void) const { return this->peer_; }
  Ptr<DgramChannel> channel(void) const { return this->channel_; }
  
  virtual bool CanSend(void) const { return FaceStatus_IsUsable(this->status()); }
  virtual bool CanReceive(void) const { return FaceStatus_IsUsable(this->status()); }
  
  virtual void Send(Ptr<Message> message);
  
  void Deliver(Ptr<Message> msg);
  virtual void Close(void);
  
  // CloseInternal closes the face immediately,
  // but does not notify DgramChannel.
  void CloseInternal(void);
  
 private:
  NetworkAddress peer_;
  Ptr<DgramChannel> channel_;
  DISALLOW_COPY_AND_ASSIGN(DgramFace);
};

// A DgramFallbackFace is a DgramFace that receives messages
// from peers with no DgramFace.
// It cannot be used for sending.
class DgramFallbackFace : public DgramFace {
 public:
  DgramFallbackFace(Ptr<DgramChannel> channel);
  virtual ~DgramFallbackFace(void) {}
  
  virtual bool CanSend(void) const { return false; }
  
  // Close closes the fallback face, and the DgramChannel.
  virtual void Close(void);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(DgramFallbackFace);
};

// A DgramChannel represents a datagram socket shared by zero or more DgramFaces.
class DgramChannel : public Element, public IPollClient {
 public:
  // fd: fd of the socket, after bind(local_addr)
  DgramChannel(int fd, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~DgramChannel(void);
  
  // GetFallbackFace returns the fallback face: "unsolicited" messages
  // (from peers without a DgramFace) are received on this face.
  Ptr<DgramFallbackFace> GetFallbackFace(void) const { return this->fallback_face_; }
  
  // GetMcastFace returns a multicast for a group.
  // One is created if it does not exist.
  Ptr<DgramFace> GetMcastFace(const NetworkAddress& group);
  
  // GetFace returns a unicast face for a peer.
  // One is created if it does not exist.
  Ptr<DgramFace> GetFace(const NetworkAddress& peer);
  
  // FaceSend sends message to face->peer() over the channel.
  // This is called by DgramFace.
  virtual void FaceSend(Ptr<DgramFace> face, Ptr<Message> message);
  
  // FaceClose removes a face.
  // It does not remove the PeerEntry.
  void FaceClose(Ptr<DgramFace> face);
  
  // PollCallback is invoked with POLLIN when there are packets
  // on the socket to read.
  virtual void PollCallback(int fd, short revents);
  
  // Close closes the channel and all associated faces.
  virtual void Close(void);
  
 protected:
  // A PeerEntry represents per-peer information.
  // If .wp()->IsStateful() is false, Ptr<WireProtocolState> is null.
  typedef std::tuple<Ptr<DgramFace>,Ptr<WireProtocolState>> PeerEntry;
  
  // MakePeerFaceOp specifies what to do with Face in MakePeer
  enum class MakePeerFaceOp {
    kNone   = 0,//no operation
    kCreate = 1,//create if not exist
    kDelete = 2 //delete if exist
  };

  // A McastEntry represents per-multicast-group information.
  struct McastEntry : public Object {
    McastEntry(AddressHashKey group) { this->group_ = group; this->face_ = nullptr; this->outstate_ = nullptr; }
    AddressHashKey group_;// group address
    Ptr<DgramFace> face_;// face
    Ptr<WireProtocolState> outstate_;// outgoing state, null if !.wp()->IsStateful()
    std::unordered_map<AddressHashKey,Ptr<WireProtocolState>> instate_;// per-peer state, null if !.wp()->IsStateful()
   private:
    DISALLOW_COPY_AND_ASSIGN(McastEntry);
  };

  bool closed_;
  int fd(void) const { return this->fd_; }
  Ptr<AddressVerifier> av(void) const { return this->av_; }
  Ptr<WireProtocol> wp(void) const { return this->wp_; }
  std::unordered_map<AddressHashKey,PeerEntry>& peers(void) { return this->peers_; }
  Ptr<Buffer> recvbuf(void) const { return this->recvbuf_; }
  const NetworkAddress& local_addr(void) const { return this->local_addr_; }

  // CloseFd closes fd
  virtual void CloseFd(void);
  
  // CreateFace makes a face for a peer.
  virtual Ptr<DgramFace> CreateFace(const AddressHashKey& hashkey, const NetworkAddress& peer);
  
  // MakePeer ensures PeerEntry exists for peer.
  // WireProtocolState is created if WireProtocol is stateful.
  PeerEntry MakePeer(const NetworkAddress& peer, MakePeerFaceOp face_op);
  
  // CreateMcastFace makes a face for a multicast group.
  // It returns null if mcast is not supported.
  virtual Ptr<DgramFace> CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group) { return nullptr; }
  
  // FindMcastEntry finds McastEntry for group,
  // or returns null if it does not exist.
  Ptr<McastEntry> FindMcastEntry(const NetworkAddress& group) { return this->FindMcastEntryInternal(group, false); }
  
  // MakeMcastEntry ensures McastEntry exists for group.
  // Face is created with CreateMcastFace.
  // outstate is created if WireProtocol is stateful.
  Ptr<McastEntry> MakeMcastEntry(const NetworkAddress& group) { return this->FindMcastEntryInternal(group, true); }
  
  // SendTo calls sendto() syscall to send one packet to the socket.
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);
  
  // ReceiveFrom calls recvfrom() syscall to read one or more packets
  // from the socket, and call DeliverPacket or DeliverMcastPacket for each packet.
  virtual void ReceiveFrom(void);
  
  // DeliverPacket decodes pkt, delivers any result messages
  // to the Face associated with peer or the fallback face.
  virtual void DeliverPacket(const NetworkAddress& peer, Ptr<BufferView> pkt);
  
  // DeliverMcastPacket does similar work as DeliverPacket
  // for packet received on a multicast group address.
  void DeliverMcastPacket(const NetworkAddress& group, const NetworkAddress& peer, Ptr<BufferView> pkt);
  virtual void DeliverMcastPacket(Ptr<McastEntry> entry, const NetworkAddress& peer, Ptr<BufferView> pkt);
  
  // DecodeAndDeliver decodes pkt, and delivers any result messages to face.
  virtual void DecodeAndDeliver(const NetworkAddress& peer, Ptr<WireProtocolState> wps, Ptr<BufferView> pkt, Ptr<DgramFace> face);
  
 private:
  int fd_;
  Ptr<AddressVerifier> av_;
  Ptr<WireProtocol> wp_;
  std::unordered_map<AddressHashKey,PeerEntry> peers_;
  Ptr<DgramFallbackFace> fallback_face_;
  std::vector<Ptr<McastEntry>> mcasts_;//there's a small number of McastEntry, so vector would be faster than unordered_map
  Ptr<Buffer> recvbuf_;
  NetworkAddress local_addr_;
  
  std::vector<Ptr<McastEntry>>& mcasts(void) { return this->mcasts_; }
  Ptr<McastEntry> FindMcastEntryInternal(const NetworkAddress& group, bool create);
  
  DISALLOW_COPY_AND_ASSIGN(DgramChannel);
};

};//namespace ndnfd
#endif//NDNFD_FACE_DGRAM_H
