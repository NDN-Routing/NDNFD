#ifndef NDNFD_FACE_ETHER_H_
#define NDNFD_FACE_ETHER_H_
#include "face/dgram.h"
#include "face/factory.h"
namespace ndnfd {

// A BpfChannel represents a socket using Berkeley Packet Filter
// to send and receive Ethernet frames.
class BpfChannel : DgramChannel {
 public:
  BpfChannel(int fd, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp);
  virtual ~BpfChannel(void) {}

 protected:
  virtual void ReceiveFrom(void);

 private:
  DISALLOW_COPY_AND_ASSIGN(BpfChannel);
};

// A EtherAddressVerifier verifies Ethernet addresses.
// Address is uint8_t[6] MAC address.
class EtherAddressVerifier : public AddressVerifier {
 public:
  virtual ~EtherAddressVerifier(void) {}
  virtual bool CheckAddress(const NetworkAddress& addr);
  virtual void NormalizeAddress(NetworkAddress* addr);
  virtual std::string AddressToString(const NetworkAddress& addr);
 private:
  DISALLOW_COPY_AND_ASSIGN(EtherAddressVerifier);
};

// A EtherFaceFactory creates Face objects for Ethernet.
class EtherFaceFactory : public FaceFactory {
 public:
  EtherFaceFactory(Ptr<WireProtocol> wp);
  virtual ~EtherFaceFactory(void) {}
  
  // MakeChannel creates a DgramChannel (BpfChannel) for Ethernet.
  Ptr<DgramChannel> MakeChannel(std::string ifname);

 private:
  Ptr<WireProtocol> wp_;
  Ptr<WireProtocol> wp(void) const { return this->wp_; }

  DISALLOW_COPY_AND_ASSIGN(EtherFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ETHER_H
