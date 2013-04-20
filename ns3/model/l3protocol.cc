#include "l3protocol.h"
#include <ns3/simulator.h>
#include "face/facemgr.h"
#include "strategy/strategy.h"
#include "ndnfdsim.h"
#include "mock_fw.h"
#include "mock_fib.h"
namespace ndnfd {

ns3::TypeId L3Protocol::GetTypeId(void) {
  static ns3::TypeId tid = ns3::TypeId("ndnfd::L3Protocol")
    .SetGroupName("NDNFD")
    .SetParent<ns3::ndn::L3Protocol>()
#ifdef NDNFD_STRATEGY_TRACE
    .AddTraceSource("InterestMcastSend", "Interest sent to multicast group", ns3::MakeTraceSourceAccessor(&L3Protocol::trace_mcast_send_))
    .AddTraceSource("InterestMcastRecv", "Interest received on multicast face", ns3::MakeTraceSourceAccessor(&L3Protocol::trace_mcast_recv_))
    .AddTraceSource("InterestUnicastSend", "Interest sent to unicast peer", ns3::MakeTraceSourceAccessor(&L3Protocol::trace_unicast_send_))
#endif
    .AddConstructor<L3Protocol>();
  return tid;
}

L3Protocol::L3Protocol(void) {
  this->global_ = nullptr;
}

L3Protocol::~L3Protocol(void) {
  if (this->global_ != nullptr) delete this->global_;
}

void L3Protocol::Init(ns3::Ptr<ns3::Node> node) {
  if (this->global_ != nullptr) return;
  NS_ASSERT_MSG(ns3::Now() >= L3Protocol::kMinStartTime(), "cannot initialize before kMinStartTime");//ContentObject Timestamp is considered invalid before kMinStartTime
  
  this->global_ = new SimGlobal(node->GetId());
  this->global_->Init();
  this->global_->set_l3(this);
#ifdef NDNFD_STRATEGY_TRACE
  this->global_->strategy()->Trace = [this] (Strategy::TraceEvt evt, Ptr<const Name> name) {
    switch (evt) {
      case Strategy::TraceEvt::kMcastSend: this->trace_mcast_send_(this, PeekPointer(name)); break;
      case Strategy::TraceEvt::kMcastRecv: this->trace_mcast_recv_(this, PeekPointer(name)); break;
      case Strategy::TraceEvt::kUnicastSend: this->trace_unicast_send_(this, PeekPointer(name)); break;
      default: break;
    }
  };
#endif

  this->AggregateObject(ns3::CreateObject<MockForwardingStrategy>());
  this->AggregateObject(ns3::CreateObject<MockFib>(this->global()));
  node->AggregateObject(this);
  
  Ptr<NdnfdSim> program = this->global()->facemgr()->New<NdnfdSim>();
  this->global()->set_program(program);
  program->Start();
}

uint32_t L3Protocol::nodeid(void) const {
  ns3::Ptr<ns3::Node> node = this->GetObject<ns3::Node>();
  if (node == nullptr) return 0xFFFFFFFF;
  return node->GetId();
}

uint32_t L3Protocol::AddFace(const ns3::Ptr<ns3::ndn::Face>& face) {
  NS_ASSERT_MSG(ns3::TypeId::LookupByName("ns3::ndn::AppFace") == face->GetInstanceTypeId(), "face is not AppFace");

  Ptr<SimAppFace> aface = this->global()->facemgr()->New<SimAppFace>(this);
  uint32_t faceid = static_cast<uint32_t>(aface->id());
  face->SetId(faceid);
  face->RegisterProtocolHandler(ns3::MakeCallback(&L3Protocol::AppReceive, this));
  
  this->global()->logging()->Log(kLLInfo, kLCSim, "L3Protocol(%" PRIu32 ")::AddFace(%" PRIxPTR ") id=%" PRI_FaceId "", this->nodeid(), PeekPointer(face), faceid);

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
  Ptr<Message> msg = this->global()->npc()->MessageFrom(p->Copy());
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
  this->global()->program()->ScheduleOnNextRun(std::bind(&SimAppFace::Deliver, aface, msg));
}

void L3Protocol::AppSend(SimAppFace* aface, const Message* msg) {
  ns3::Ptr<ns3::Packet> p = this->global()->npc()->MessageTo(msg);
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
