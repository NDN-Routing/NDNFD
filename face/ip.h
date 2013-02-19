#ifndef NDNFD_FACE_IP_H_
#define NDNFD_FACE_IP_H_
#include "face/dgram.h"
#include "face/stream.h"
#include "face/factory.h"
namespace ndnfd {

// An IpAddressVerifier verifies sockaddr_in or sockaddr_in6 addresses.
class IpAddressVerifier : public AddressVerifier {
 public:
  virtual ~IpAddressVerifier(void) {}
  virtual bool CheckAddress(const NetworkAddress& addr);
  virtual void NormalizeAddress(NetworkAddress* addr);
  virtual std::string AddressToString(const NetworkAddress& addr);
 private:
  DISALLOW_COPY_AND_ASSIGN(IpAddressVerifier);
};

// A UdpFaceFactory creates Face objects for UDP.
// Address is either sockaddr_in or sockaddr_in6.
class UdpFaceFactory : public FaceFactory {
 public:
  UdpFaceFactory(Ptr<WireProtocol> wp);
  virtual ~UdpFaceFactory(void) {}
  
  // MakeChannel creates a DgramChannel for UDP over IPv4 or IPv6.
  Ptr<DgramChannel> MakeChannel(const NetworkAddress& local_addr);

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
  
  // MakeListener creates a StreamListener for TCP over IPv4 or IPv6.
  Ptr<StreamListener> MakeListener(const NetworkAddress& local_addr);

  // Connect creates a StreamFace for TCP over IPv4 or IPv6,
  // and connects to a remote peer.
  Ptr<StreamFace> Connect(const NetworkAddress& remote_addr);

 private:
  Ptr<IpAddressVerifier> av_;
  DISALLOW_COPY_AND_ASSIGN(TcpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_IP_H
