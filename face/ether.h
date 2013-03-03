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
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);
  virtual void ReceiveFrom(void);

 private:
  DISALLOW_COPY_AND_ASSIGN(BpfChannel);
};

// A EtherAddressVerifier verifies Ethernet addresses.
// Address is uint8_t[6] MAC address.
class EtherAddressVerifier : public AddressVerifier {
 public:
  EtherAddressVerifier(void) {}
  virtual ~EtherAddressVerifier(void) {}
  virtual bool Check(const NetworkAddress& addr);
  virtual AddressHashKey GetHashKey(const NetworkAddress& addr);
  virtual std::string ToString(const NetworkAddress& addr);
 private:
  DISALLOW_COPY_AND_ASSIGN(EtherAddressVerifier);
};

// A EtherFaceFactory creates Face objects for Ethernet.
class EtherFaceFactory : public FaceFactory {
 public:
  EtherFaceFactory(Ptr<WireProtocol> wp);
  virtual ~EtherFaceFactory(void) {}
  
  // Channel creates a DgramChannel for Ethernet.
  Ptr<DgramChannel> Channel(std::string ifname);

 private:
  Ptr<WireProtocol> wp_;
  Ptr<WireProtocol> wp(void) const { return this->wp_; }

  DISALLOW_COPY_AND_ASSIGN(EtherFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ETHER_H
