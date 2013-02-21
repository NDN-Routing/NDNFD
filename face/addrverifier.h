#ifndef NDNFD_FACE_ADDRVERIFIER_H_
#define NDNFD_FACE_ADDRVERIFIER_H_
#include "face/faceid.h"
namespace ndnfd {

// An AddressVerifier subclass knows about the address format of a lower protocol,
// such as IPv4 or Ethernet.
class AddressVerifier : public Object {
 public:
  virtual ~AddressVerifier(void) {}
  
  // Check checks whether addr is valid in lower protocol.
  virtual bool Check(const NetworkAddress& addr) =0;
  
  // Normalize returns an address that is suitable to use as a hash key.
  virtual NetworkAddress Normalize(const NetworkAddress& addr) { return addr; }
  
  // IsLocal returns true if addr represents localhost.
  virtual bool IsLocal(const NetworkAddress& addr) { return false; };
  
  // AreSameHost returns true if a and b represent the same host (but not necessarily same endpoint).
  virtual bool AreSameHost(const NetworkAddress& a, const NetworkAddress& b) { return false; }

  // ToString returns a human readable string representation of an address.
  virtual std::string ToString(const NetworkAddress& addr) =0;

 protected:
  AddressVerifier(void) {}
  
 private:
  DISALLOW_COPY_AND_ASSIGN(AddressVerifier);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ADDRVERIFIER_H
