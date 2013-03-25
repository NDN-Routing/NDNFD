#include "l3protocol.h"
#include <ns3/node.h>
#include <ns3/ndn-forwarding-strategy.h>
#include <ns3/ndn-pit.h>
namespace ndnfd {

class FakeForwardingStrategy : public ns3::ndn::ForwardingStrategy {
 protected:
  virtual bool DoPropagateInterest(ns3::Ptr<ns3::ndn::Face> inFace, ns3::Ptr<const ns3::ndn::Interest> header, ns3::Ptr<const ns3::Packet> origPacket, ns3::Ptr<ns3::ndn::pit::Entry> pitEntry) { return false; }
};

class FakePit : public ns3::ndn::Pit {
 public:
  virtual ns3::Ptr<ns3::ndn::pit::Entry> Lookup(const ns3::ndn::ContentObject& header) { return nullptr; }
  virtual ns3::Ptr<ns3::ndn::pit::Entry> Lookup(const ns3::ndn::Interest& header) { return nullptr; }
  virtual ns3::Ptr<ns3::ndn::pit::Entry> Find(const ns3::ndn::Name& prefix) { return nullptr; }
  virtual ns3::Ptr<ns3::ndn::pit::Entry> Create(ns3::Ptr<const ns3::ndn::Interest> header) { return nullptr; }
  virtual void MarkErased(ns3::Ptr<ns3::ndn::pit::Entry> entry) {}
  virtual void Print(std::ostream& os) const {}
  virtual uint32_t GetSize() const { return 0; }
  virtual ns3::Ptr<ns3::ndn::pit::Entry> Begin() { return nullptr; }
  virtual ns3::Ptr<ns3::ndn::pit::Entry> End() { return nullptr; }
  virtual ns3::Ptr<ns3::ndn::pit::Entry> Next(ns3::Ptr<ns3::ndn::pit::Entry>) { return nullptr; }
};

ns3::TypeId L3Protocol::GetTypeId(void) {
  static ns3::TypeId tid = ns3::TypeId("ndnfd::L3Protocol")
    .SetGroupName("NDNFD")
    .SetParent<ns3::ndn::L3Protocol>()
    .AddConstructor<L3Protocol> ();
  return tid;
}

void L3Protocol::NotifyNewAggregate(void) {
  ns3::Ptr<ns3::Node> node = this->GetObject<ns3::Node>();
  if (node->GetObject<ns3::ndn::ForwardingStrategy>() == nullptr) {
    node->AggregateObject(ns3::CreateObject<FakeForwardingStrategy>());
  }
  if (node->GetObject<ns3::ndn::Pit>() == nullptr) {
    node->AggregateObject(ns3::CreateObject<FakePit>());
  }
  this->ns3::ndn::L3Protocol::NotifyNewAggregate();
}

uint32_t L3Protocol::AddFace(const ns3::Ptr<ns3::ndn::Face> face) {
  NS_ASSERT_MSG(ns3::TypeId::LookupByName("ns3::ndn::AppFace") == face->GetInstanceTypeId(), "face is not AppFace");
  uint32_t index = this->ns3::ndn::L3Protocol::AddFace(face);
  // TODO add face in NDNFD
  // TODO face->SetId
  face->RegisterProtocolHandler(ns3::MakeCallback(&L3Protocol::AppReceive, this));
  return index;
}

void L3Protocol::RemoveFace(ns3::Ptr<ns3::ndn::Face> face) {
  // TODO delete face in NDNFD
  this->ns3::ndn::L3Protocol::RemoveFace(face);
}

void L3Protocol::AppReceive(const ns3::Ptr<ns3::ndn::Face>& face, const ns3::Ptr<const ns3::Packet>& p) {
  // TODO convert p to msg
  // TODO deliver msg to NDNFD
}

void L3Protocol::AppSend(FaceId faceid, const Ptr<Message> msg) {
  // TODO convert msg to p
  // TODO AppFace::Send(p)
}

};//namespace ndnfd
