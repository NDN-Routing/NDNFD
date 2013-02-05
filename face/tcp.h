#ifndef NDNFD_FACE_TCP_H_
#define NDNFD_FACE_TCP_H_
#include "face/stream.h"
namespace ndnfd {

// A TcpFaceFactory creates Face objects for TCP.
class TcpFaceFactory : public Element, public IAddressVerifier {
  public:
    bool CheckAddress(const NetworkAddress& addr);
    void NormalizeAddress(NetworkAddress& addr);

    // MakeChannel creates a StreamListener for TCP over IPv4 or IPv6.
    // local_addr should be either sockaddr_in or sockaddr_in6.
    Ptr<StreamListener> MakeListener(const NetworkAddress& local_addr);

    // MakeChannel creates a StreamFace for TCP over IPv4 or IPv6.
    // remote_addr should be either sockaddr_in or sockaddr_in6.
    Ptr<StreamFace> MakeConnection(const NetworkAddress& remote_addr);

  private:
    DISALLOW_COPY_AND_ASSIGN(TcpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_TCP_H
