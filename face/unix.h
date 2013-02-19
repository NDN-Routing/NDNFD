#ifndef NDNFD_FACE_UNIX_H_
#define NDNFD_FACE_UNIX_H_
#include "face/stream.h"
namespace ndnfd {

// A UnixAddressVerifier verifies UNIX socket addresses.
// Address is sockaddr_un.
class UnixAddressVerifier : public AddressVerifier {
 public:
  virtual ~UnixAddressVerifier(void) {}
  virtual bool CheckAddress(const NetworkAddress& addr);
  virtual void NormalizeAddress(NetworkAddress* addr);
  virtual std::string AddressToString(const NetworkAddress& addr);
 private:
  DISALLOW_COPY_AND_ASSIGN(UnixAddressVerifier);
};

// A UnixFaceFactory creates Face objects for UNIX sockets.
class UnixFaceFactory : public Element {
 public:
  UnixFaceFactory(Ptr<WireProtocol> wp);
  virtual ~UnixFaceFactory(void) {}
  
  // MakeListener creates a StreamListener for TCP over IPv4 or IPv6.
  Ptr<StreamListener> MakeListener(const NetworkAddress& local_addr);

 private:
  Ptr<WireProtocol> wp_;
  Ptr<WireProtocol> wp(void) const { return this->wp_; }

  DISALLOW_COPY_AND_ASSIGN(UnixFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UNIX_H
