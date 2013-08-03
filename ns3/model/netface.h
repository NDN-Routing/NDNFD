#ifndef NDNFD_NS3_MODEL_NETFACE_H_
#define NDNFD_NS3_MODEL_NETFACE_H_
#include "face/ether.h"
#include <ns3/net-device.h>
#include "l3protocol.h"
namespace ndnfd {

class SimNetChannel : public DgramChannel {
 public:
  SimNetChannel(ns3::Ptr<ns3::NetDevice> nic, uint16_t ether_type, Ptr<const AddressVerifier> av, Ptr<const WireProtocol> wp);
  virtual void Init(void);
  virtual ~SimNetChannel(void) {}
  
  static NetworkAddress ConvertAddress(const ns3::Address& addr);
  static ns3::Address ConvertAddress(const NetworkAddress& addr);

  virtual void FaceSend(Ptr<DgramFace> face, Ptr<const Message> message);

 protected:
  virtual void CloseFd(void);
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);
  virtual void ReceiveFrom(void) { assert(false); }//never called because PollMgr is not used
  
  virtual Ptr<DgramFace> CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group);
  virtual void DeliverMessage(Ptr<DgramFace> face, Ptr<Message> message);

 private:
  ns3::Ptr<ns3::NetDevice> nic_;
  uint16_t ether_type_;
  
  // NicReceive is attached by ns3::Node::RegisterProtocolHandler
  void NicReceive(ns3::Ptr<ns3::NetDevice> device, ns3::Ptr<const ns3::Packet> packet, uint16_t protocol, const ns3::Address& sender, const ns3::Address& receiver, ns3::NetDevice::PacketType packetType);
  
  DISALLOW_COPY_AND_ASSIGN(SimNetChannel);
};

// A SimFaceFactory creates Face objects in ns3.
class SimFaceFactory : public FaceFactory {
 public:
  SimFaceFactory(void);
  virtual ~SimFaceFactory(void) {}
  
  // ListNICs returns a list of net devices.
  std::vector<ns3::Ptr<ns3::NetDevice>> ListNICs(void);
  
  // Channel creates a DgramChannel for NIC.
  Ptr<DgramChannel> Channel(ns3::Ptr<ns3::NetDevice> dev, uint16_t ether_type);

 private:
  Ptr<EtherAddressVerifier> av_;

  DISALLOW_COPY_AND_ASSIGN(SimFaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_NETFACE_H_
