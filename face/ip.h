#ifndef NDNFD_FACE_IP_H_
#define NDNFD_FACE_IP_H_
#include <netinet/ip.h>
#include "face/dgram.h"
#include "face/stream.h"
#include "face/factory.h"
namespace ndnfd {

// An IpAddressVerifier verifies sockaddr_in or sockaddr_in6 addresses.
class IpAddressVerifier : public AddressVerifier {
 public:
  IpAddressVerifier(void) {}
  virtual ~IpAddressVerifier(void) {}
  virtual bool Check(const NetworkAddress& addr);
  virtual AddressHashKey GetHashKey(const NetworkAddress& addr);
  virtual bool IsLocal(const NetworkAddress& addr);
  virtual bool AreSameHost(const NetworkAddress& a, const NetworkAddress& b);
  virtual std::string ToString(const NetworkAddress& addr);
  static std::tuple<bool,NetworkAddress> Parse(std::string s);
 private:
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

 private:
  Ptr<IpAddressVerifier> av_;
  DISALLOW_COPY_AND_ASSIGN(UdpFaceFactory);
};

// A TcpFaceFactory creates Face objects for TCP.
// Address is either sockaddr_in or sockaddr_in6.
class TcpFaceFactory : public FaceFactory {
 public:
  TcpFaceFactory(Ptr<WireProtocol> wp);
  virtual ~TcpFaceFactory(void) {}
  
  // Listen creates a StreamListener for TCP over IPv4 or IPv6.
  Ptr<StreamListener> Listen(const NetworkAddress& local_addr);

  // Connect creates a StreamFace for TCP over IPv4 or IPv6,
  // and connects to a remote peer.
  Ptr<StreamFace> Connect(const NetworkAddress& remote_addr);

 private:
  Ptr<IpAddressVerifier> av_;
  DISALLOW_COPY_AND_ASSIGN(TcpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_IP_H
