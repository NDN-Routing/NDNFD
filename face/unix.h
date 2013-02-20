#ifndef NDNFD_FACE_UNIX_H_
#define NDNFD_FACE_UNIX_H_
#include <sys/un.h>
#include "face/stream.h"
#include "face/factory.h"
namespace ndnfd {

// A UnixAddressVerifier verifies UNIX socket addresses.
// Address is sockaddr_un.
class UnixAddressVerifier : public AddressVerifier {
 public:
  UnixAddressVerifier(void) {}
  virtual ~UnixAddressVerifier(void) {}
  virtual bool Check(const NetworkAddress& addr);
  virtual NetworkAddress Normalize(const NetworkAddress& addr);
  virtual std::string ToString(const NetworkAddress& addr);
 private:
  DISALLOW_COPY_AND_ASSIGN(UnixAddressVerifier);
};

// A UnixFaceFactory creates Face objects for UNIX sockets.
class UnixFaceFactory : public FaceFactory {
 public:
  UnixFaceFactory(Ptr<WireProtocol> wp);
  virtual ~UnixFaceFactory(void) {}
  
  // Listen creates a StreamListener for UNIX socket.
  Ptr<StreamListener> Listen(const std::string& local_socket);

 private:
  Ptr<UnixAddressVerifier> av_;
  DISALLOW_COPY_AND_ASSIGN(UnixFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UNIX_H
