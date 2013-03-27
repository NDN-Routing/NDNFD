#include "l3protocol.h"
#include <ns3/simulator.h>
#include <ns3/ndn-forwarding-strategy.h>
#include <ns3/ndn-pit.h>
#include "face/facemgr.h"
#include "ndnfdsim.h"
namespace ndnfd {

class MockForwardingStrategy : public ns3::ndn::ForwardingStrategy {
 public:
  static ns3::TypeId GetTypeId(void) {
    static ns3::TypeId tid = ns3::TypeId("ndnfd::MockForwardingStrategy")
      .SetGroupName("NDNFD")
      .SetParent<ns3::ndn::ForwardingStrategy>()
      .AddConstructor<MockForwardingStrategy>();
    return tid;
  }
  MockForwardingStrategy(void) {}
  virtual ~MockForwardingStrategy(void) {}
 protected:
  virtual bool DoPropagateInterest(ns3::Ptr<ns3::ndn::Face> inFace, ns3::Ptr<const ns3::ndn::Interest> header, ns3::Ptr<const ns3::Packet> origPacket, ns3::Ptr<ns3::ndn::pit::Entry> pitEntry) { return false; }
};

class MockPit : public ns3::ndn::Pit {
 public:
  static ns3::TypeId GetTypeId(void) {
    static ns3::TypeId tid = ns3::TypeId("ndnfd::MockPit")
      .SetGroupName("NDNFD")
      .SetParent<ns3::ndn::Pit>()
      .AddConstructor<MockPit>();
    return tid;
  }
  MockPit(void) {}
  virtual ~MockPit(void) {}

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
    .AddConstructor<L3Protocol>();
  return tid;
}

L3Protocol::L3Protocol(void) {
  this->global_ = nullptr;
  Ptr<NdnsimPacketConverter> npc = new NdnsimPacketConverter();
  this->npc_ = GetPointer(npc);
  this->program_ = nullptr;
}

L3Protocol::~L3Protocol(void) {
  if (this->program_ != nullptr) this->program_->Unref();
  this->npc_->Unref();
  if (this->global_ != nullptr) delete this->global_;
}

void L3Protocol::Init(ns3::Ptr<ns3::Node> node) {
  if (this->global_ != nullptr) return;
  NS_ASSERT_MSG(ns3::Now() >= L3Protocol::kMinStartTime(), "cannot initialize before kMinStartTime");//ContentObject Timestamp is considered invalid before kMinStartTime
  
  this->global_ = new SimGlobal();
  this->global_->Init();
  this->global_->set_l3(this);

  this->AggregateObject(ns3::CreateObject<MockForwardingStrategy>());
  this->AggregateObject(ns3::CreateObject<MockPit>());
  node->AggregateObject(this);
  
  Ptr<NdnfdSim> program = this->global()->facemgr()->New<NdnfdSim>(node->GetId());
  this->program_ = GetPointer(program);
  program->Start();
}

uint32_t L3Protocol::nodeid(void) const {
  ns3::Ptr<ns3::Node> node = this->GetObject<ns3::Node>();
  if (node == nullptr) return 0xFFFFFFFF;
  return node->GetId();
}

uint32_t L3Protocol::AddFace(const ns3::Ptr<ns3::ndn::Face> face) {
  NS_ASSERT_MSG(ns3::TypeId::LookupByName("ns3::ndn::AppFace") == face->GetInstanceTypeId(), "face is not AppFace");

  Ptr<SimAppFace> aface = this->global()->facemgr()->New<SimAppFace>(this);
  uint32_t faceid = static_cast<uint32_t>(aface->id());
  face->SetId(faceid);
  face->RegisterProtocolHandler(ns3::MakeCallback(&L3Protocol::AppReceive, this));

  this->facelist_[faceid] = face;
  return faceid;
}

void L3Protocol::RemoveFace(ns3::Ptr<ns3::ndn::Face> face) {
  uint32_t faceid = face->GetId();
  this->facelist_.erase(faceid);

  Ptr<Face> nface = this->global()->facemgr()->GetFace(static_cast<FaceId>(faceid));
  if (nface == nullptr) {
    this->global()->logging()->Log(kLLWarn, kLCSim, "L3Protocol(%" PRIu32 ")::RemoveFace(%" PRIu32 ") SimAppFace not found", this->nodeid(), face->GetId());
    return;
  }
  nface->Close();
}

ns3::Ptr<ns3::ndn::Face> L3Protocol::GetFaceById(uint32_t id) const {
  auto it = this->facelist_.find(id);
  if (it == this->facelist_.cend()) return nullptr;
  return it->second;
}

ns3::Ptr<ns3::ndn::Face> L3Protocol::GetFace(uint32_t index) const {
  if (index >= this->facelist_.size()) return nullptr;
  auto it = this->facelist_.cbegin();
  for (; index > 0; --index) ++it;
  return it->second;
}

void L3Protocol::AppReceive(const ns3::Ptr<ns3::ndn::Face>& face, const ns3::Ptr<const ns3::Packet>& p) {
  Ptr<Message> msg = this->npc_->MessageFrom(p->Copy());
  if (msg == nullptr) {
    this->global()->logging()->Log(kLLWarn, kLCSim, "L3Protocol(%" PRIu32 ")::AppReceive(%" PRIu32 ") cannot convert packet", this->nodeid(), face->GetId());
    return;
  }

  Ptr<Face> nface = this->global()->facemgr()->GetFace(static_cast<FaceId>(face->GetId()));
  if (nface == nullptr) {
    this->global()->logging()->Log(kLLWarn, kLCSim, "L3Protocol(%" PRIu32 ")::AppReceive(%" PRIu32 ") SimAppFace not found", this->nodeid(), face->GetId());
    return;
  }
  SimAppFace* aface = static_cast<SimAppFace*>(PeekPointer(nface));
  aface->Deliver(msg);
}

void L3Protocol::AppSend(SimAppFace* aface, const Message* msg) {
  ns3::Ptr<ns3::Packet> p = this->npc_->MessageTo(msg);
  if (p == nullptr) {
    this->global()->logging()->Log(kLLWarn, kLCSim, "L3Protocol(%" PRIu32 ")::AppSend(%" PRI_FaceId ") cannot convert message", this->nodeid(), aface->id());
    return;
  }
  
  ns3::Ptr<ns3::ndn::Face> face = this->GetFaceById(static_cast<uint32_t>(aface->id()));
  if (face == nullptr) {
    this->global()->logging()->Log(kLLWarn, kLCSim, "L3Protocol(%" PRIu32 ")::AppSend(%" PRIu32 ") ndnSIM Face not found", this->nodeid(), aface->id());
    return;
  }

  face->Send(p);
}

};//namespace ndnfd
