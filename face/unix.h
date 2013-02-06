#ifndef NDNFD_FACE_UNIX_H_
#define NDNFD_FACE_UNIX_H_
#include "face/stream.h"
namespace ndnfd {

// A UnixFaceFactory creates Face objects for UNIX sockets.
// Address is sockaddr_un.
class UnixFaceFactory : public Element, public IAddressVerifier {
  public:
    UnixFaceFactory(Ptr<WireProtocol> wp);
    
    // MakeListener creates a StreamListener for TCP over IPv4 or IPv6.
    Ptr<StreamListener> MakeListener(const NetworkAddress& local_addr);

    bool CheckAddress(const NetworkAddress& addr);
    void NormalizeAddress(NetworkAddress& addr);

  private:
    Ptr<WireProtocol> wp_;

    Ptr<WireProtocol> wp(void) const { return this->wp_; }

    DISALLOW_COPY_AND_ASSIGN(UnixFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UNIX_H
