#ifndef NDNFD_NS3_MODEL_NETFACE_H_
#define NDNFD_NS3_MODEL_NETFACE_H_
#include "face/dgram.h"
#include <ns3/net-device.h>
#include "l3protocol.h"
namespace ndnfd {

class SimNetChannel : public DgramChannel {
 public:
  static const uint16_t EtherProtocol;
  
  SimNetChannel(ns3::Ptr<ns3::NetDevice> nic, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp);
  virtual void Init(void);
  virtual ~SimNetChannel(void) {}
  
  static NetworkAddress ConvertAddress(const ns3::Address& addr);
  static ns3::Address ConvertAddress(const NetworkAddress& addr);

 protected:
  virtual void CloseFd(void);
  virtual void SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt);
  virtual void ReceiveFrom(void) { assert(false); }//never called because PollMgr is not used
  
  virtual Ptr<DgramFace> CreateMcastFace(const AddressHashKey& hashkey, const NetworkAddress& group);

 private:
  ns3::Ptr<ns3::NetDevice> nic_;
  
  // NicReceive is attached by ns3::Node::RegisterProtocolHandler
  void NicReceive(ns3::Ptr<ns3::NetDevice> device, ns3::Ptr<const ns3::Packet> packet, uint16_t protocol, const ns3::Address& sender, const ns3::Address& receiver, ns3::NetDevice::PacketType packetType);
  
  DISALLOW_COPY_AND_ASSIGN(SimNetChannel);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_FACE_H_
