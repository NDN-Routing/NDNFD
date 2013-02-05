#ifndef NDNFD_FACE_UNIX_H_
#define NDNFD_FACE_UNIX_H_
#include "face/stream.h"
namespace ndnfd {

// A UnixFaceFactory creates Face objects for UNIX sockets.
class UnixFaceFactory : public Element, public IAddressVerifier {
  public:
    bool CheckAddress(const NetworkAddress& addr);
    void NormalizeAddress(NetworkAddress& addr);

    // MakeChannel creates a StreamListener for TCP over IPv4 or IPv6.
    // local_addr should be either sockaddr_un.
    Ptr<StreamListener> MakeListener(const NetworkAddress& local_addr);

  private:
    DISALLOW_COPY_AND_ASSIGN(UnixFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UNIX_H
