#ifndef NDNFD_FACE_ETHER_H_
#define NDNFD_FACE_ETHER_H_
#include "face/dgram.h"
#include "face/factory.h"
namespace ndnfd {

// A BpfChannel represents a socket using Berkeley Packet Filter
// to send and receive Ethernet frames.
class BpfChannel : DgramChannel {
 public:
  BpfChannel(int fd, Ptr<IAddressVerifier> av, Ptr<WireProtocol> wp);

 protected:
  virtual void ReceiveFrom(void);

 private:
  DISALLOW_COPY_AND_ASSIGN(BpfChannel);
};

// A EtherFaceFactory creates Face objects for Ethernet.
// Address is uint8_t[6] MAC address.
class EtherFaceFactory : public FaceFactory, public IAddressVerifier {
 public:
  EtherFaceFactory(Ptr<WireProtocol> wp);
  
  // MakeChannel creates a DgramChannel (BpfChannel) for Ethernet.
  Ptr<DgramChannel> MakeChannel(std::string ifname);

  virtual bool CheckAddress(const NetworkAddress& addr);
  virtual void NormalizeAddress(NetworkAddress& addr);
  
 private:
  Ptr<WireProtocol> wp_;
  Ptr<WireProtocol> wp(void) const { return this->wp_; }

  DISALLOW_COPY_AND_ASSIGN(EtherFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_ETHER_H
