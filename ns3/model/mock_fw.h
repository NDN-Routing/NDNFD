#ifndef NDNFD_NS3_MODEL_MOCK_FW_H_
#define NDNFD_NS3_MODEL_MOCK_FW_H_
#include <ns3/ndn-forwarding-strategy.h>
namespace ndnfd {

class MockForwardingStrategy : public ns3::ndn::ForwardingStrategy {
 public:
  static ns3::TypeId GetTypeId(void);
  MockForwardingStrategy(void) {}
  virtual ~MockForwardingStrategy(void) {}

 protected:
  virtual bool DoPropagateInterest(ns3::Ptr<ns3::ndn::Face> inFace, ns3::Ptr<const ns3::ndn::Interest> header, ns3::Ptr<const ns3::Packet> origPacket, ns3::Ptr<ns3::ndn::pit::Entry> pitEntry) { NS_ASSERT(false); return false; }
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_MOCK_FW_H_
