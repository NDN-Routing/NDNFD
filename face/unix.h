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
  virtual std::string proto_name(void) const { return "UNIX"; }
  virtual bool Check(const NetworkAddress& addr) const;
  virtual std::string ToString(const NetworkAddress& addr) const;
 private:
  DISALLOW_COPY_AND_ASSIGN(UnixAddressVerifier);
};

// A UnixListener is a stream listener for UNIX socket,
// that unlinks local socket when closing.
class UnixListener : public StreamListener {
 public:
  UnixListener(int fd, const NetworkAddress& local_addr, Ptr<const AddressVerifier> av, Ptr<const WireProtocol> wp, const std::string& local_socket)
    : StreamListener(fd, local_addr, av, wp) {}
  
  virtual void Close(void);
  
 private:
  DISALLOW_COPY_AND_ASSIGN(UnixListener);
};

// A UnixFaceFactory creates Face objects for UNIX sockets.
class UnixFaceFactory : public FaceFactory {
 public:
  explicit UnixFaceFactory(Ptr<WireProtocol> wp);
  virtual ~UnixFaceFactory(void) {}
  
  // Listen creates a StreamListener for UNIX socket.
  Ptr<StreamListener> Listen(const std::string& local_socket);

 private:
  Ptr<UnixAddressVerifier> av_;
  DISALLOW_COPY_AND_ASSIGN(UnixFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UNIX_H
