#ifndef NDNFD_FACE_ADDRVERIFIER_H_
#define NDNFD_FACE_ADDRVERIFIER_H_
#include "face/faceid.h"
namespace ndnfd {

// An AddressVerifier subclass knows about the address format of a lower protocol,
// such as IPv4 or Ethernet.
class AddressVerifier : public Object {
 public:
  virtual ~AddressVerifier(void) {}
  
  // CheckAddress checks whether addr is valid in lower protocol.
  virtual bool CheckAddress(const NetworkAddress& addr) =0;
  
  // NormalizeAddress clears certains fields in addr so that it is suitable to use as a hash key.
  virtual void NormalizeAddress(NetworkAddress* addr) {}

  // AddressToString returns a human readable string representation of an address.
  virtual std::string AddressToString(const NetworkAddress& addr) =0;
  
 private:
  DISALLOW_COPY_AND_ASSIGN(AddressVerifier);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ADDRVERIFIER_H
