#ifndef NDNFD_FACE_UDP_H_
#define NDNFD_FACE_UDP_H_
#include "face/dgram.h"
namespace ndnfd {

// A UdpFaceFactory creates Face objects for UDP.
class UdpFaceFactory : public Element, public IAddressVerifier {
  public:
    bool CheckAddress(const NetworkAddress& addr);
    void NormalizeAddress(NetworkAddress& addr);
    
    // MakeChannel creates a DgramChannel for UDP over IPv4 or IPv6.
    // local_addr should be either sockaddr_in or sockaddr_in6.
    Ptr<DgramChannel> MakeChannel(const NetworkAddress& local_addr);

  private:
    DISALLOW_COPY_AND_ASSIGN(UdpFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_UDP_H
