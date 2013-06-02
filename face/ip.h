#ifndef NDNFD_FACE_IP_H_
#define NDNFD_FACE_IP_H_

#ifdef __FreeBSD__
// FreeBSD's netinet/ip.h depends on these
#include <sys/types.h>
#include <netinet/in.h>
//#include <sys/endian.h>
#endif

#include <netinet/ip.h>
#include "face/dgram.h"
#include "face/stream.h"
#include "face/factory.h"
#include "face/addrtable.h"
namespace ndnfd {

// An IpAddressVerifier verifies sockaddr_in or sockaddr_in6 addresses.
class IpAddressVerifier : public AddressVerifier {
 public:
  IpAddressVerifier(const std::string& proto_name) : proto_name_(proto_name) {}
  virtual ~IpAddressVerifier(void) {}
  virtual std::string proto_name(void) const { return this->proto_name_; }
  virtual bool Check(const NetworkAddress& addr) const;
  virtual AddressHashKey GetHashKey(const NetworkAddress& addr) const;
  virtual bool IsLocal(const NetworkAddress& addr) const;
  static bool IsMcast(const NetworkAddress& addr);
  virtual bool AreSameHost(const NetworkAddress& a, const NetworkAddress& b) const;
  std::string IpToString(const NetworkAddress& addr) const;
  uint16_t GetPort(const NetworkAddress& addr) const;
  virtual std::string ToString(const NetworkAddress& addr) const;
  static std::tuple<bool,NetworkAddress> Parse(std::string s);
 private:
  std::string proto_name_;
  DISALLOW_COPY_AND_ASSIGN(IpAddressVerifier);
};

// A UdpFaceFactory creates Face objects for UDP.
// Address is either sockaddr_in or sockaddr_in6.
class UdpFaceFactory : public FaceFactory {
 public:
  UdpFaceFactory(Ptr<WireProtocol> wp);
  virtual ~UdpFaceFactory(void) {}

  // Channel creates a DgramChannel for UDP over IPv4 or IPv6.
  Ptr<DgramChannel> Channel(const NetworkAddress& local_addr);
  
  // ListLocalAddresses returns a list of local NIC addresses.
  std::vector<NetworkAddress> ListLocalAddresses(void) const;
  
  // McastFace joins a UDP multicast group.
  // Each unique local_addr can only join one group,
  // otherwise unexpected behavior may occur.
  Ptr<DgramFace> McastFace(const NetworkAddress& local_addr, const NetworkAddress& group_addr, uint8_t ttl);

 private:
  std::tuple<bool,int> MakeBoundSocket(const NetworkAddress& local_addr);
  
  Ptr<IpAddressVerifier> av_;
  DISALLOW_COPY_AND_ASSIGN(UdpFaceFactory);
};

// A UdpSingleMcastChannel joins a UDP multicast group on local_addr.
// Only one group_addr per local_addr is supported, and local_addr cannot be same as UDP unicast channel.
// This implementation does not attempt to identify destination address of incoming packet,
// and accounts every incoming packet to group_addr.
class UdpSingleMcastChannel : public DgramChannel {
 public:
  UdpSingleMcastChannel(int recv_fd, int send_fd, const NetworkAddress& local_addr, const NetworkAddress& group_addr, Ptr<const AddressVerifier> av, Ptr<const WireProtocol> wp);
  virtual ~UdpSingleMcastChannel(void) {}

 protected:
  class UdpSingleMcastFace : public DgramFace {
   public:
    UdpSingleMcastFace(Ptr<DgramChannel> channel, const NetworkAddress& peer) : DgramFace(channel, peer) { this->set_kind(FaceKind::kMulticast); }
    virtual ~UdpSingleMcastFace(void) {}
    virtual void Close(void) { this->channel()->Close(); }
   private:
    DISALLOW_COPY_AND_ASSIGN(UdpSingleMcastFace);
  };

  virtual Ptr<DgramFace> CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group);
  virtual void DeliverPacket(const NetworkAddress& peer, Ptr<BufferView> pkt);
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);

 private:
  int send_fd_;
  NetworkAddress group_addr_;
  Ptr<McastEntry> group_entry_;
  DISALLOW_COPY_AND_ASSIGN(UdpSingleMcastChannel);
};

// A TcpFaceFactory creates Face objects for TCP over IPv4 or IPv6.
// Address is either sockaddr_in or sockaddr_in6.
class TcpFaceFactory : public FaceFactory {
 public:
  TcpFaceFactory(Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~TcpFaceFactory(void) {}
  
  // Listen creates a StreamListener for TCP.
  Ptr<StreamListener> Listen(const NetworkAddress& local_addr);

  // Connect creates a StreamFace for TCP, and connects to a remote peer.
  // If an active outgoing TCP face to remote_addr is found, that is returned.
  Ptr<StreamFace> Connect(const NetworkAddress& remote_addr);

  // FindFace returns an active outgoing TCP face to remote_addr,
  // or null if it does not exist.
  Ptr<StreamFace> FindFace(const NetworkAddress& remote_addr) const;

 private:
  Ptr<IpAddressVerifier> av_;
  Ptr<FaceAddressTable> fat_;

  // DoConnect makes a StreamFace and connects to remote_addr;
  Ptr<StreamFace> DoConnect(const NetworkAddress& remote_addr);

  DISALLOW_COPY_AND_ASSIGN(TcpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_IP_H
