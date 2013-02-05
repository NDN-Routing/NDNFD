#ifndef NDNFD_FACE_ADDRVERIFIER_H_
#define NDNFD_FACE_ADDRVERIFIER_H_
#include "face/faceid.h"
namespace ndnfd {

// An IAddressVerifier implementor knows about the address format of a lower protocol,
// such as IPv4 or Ethernet.
class IAddressVerifier {
  public:
    // CheckAddress checks whether addr is valid in lower protocol.
    virtual bool CheckAddress(const NetworkAddress& addr) =0;
    
    // NormalizeAddress clears certains fields in addr so that it is suitable to use as a hash key.
    virtual void NormalizeAddress(NetworkAddress& addr) {}
};

};//namespace ndnfd
#endif//NDNFD_FACE_ADDRVERIFIER_H
