#include "mock_fib.h"
#include "../utils/pktconv.h"
#include "core/nameprefix_table.h"
namespace ndnfd {

ns3::TypeId MockFib::GetTypeId(void) {
  static ns3::TypeId tid = ns3::TypeId("ndnfd::MockFib")
    .SetGroupName("NDNFD")
    .SetParent<ns3::ndn::Fib>();
  return tid;
}

ns3::Ptr<ns3::ndn::fib::Entry> MockFib::Add(const ns3::ndn::Name& prefix, ns3::Ptr<ns3::ndn::Face> face, int32_t metric) {
  ns3::Ptr<const ns3::ndn::Name> prefix_ptr(&prefix);
  return this->Add(prefix_ptr, face, metric);
}

ns3::Ptr<ns3::ndn::fib::Entry> MockFib::Add(const ns3::Ptr<const ns3::ndn::Name>& prefix, ns3::Ptr<ns3::ndn::Face> face, int32_t metric) {
  Ptr<Name> name = this->global()->npc()->NameFrom(*prefix);
  Ptr<NamePrefixEntry> npe = this->global()->npt()->Seek(name);
  Ptr<ForwardingEntry> f = npe->SeekForwarding(static_cast<FaceId>(face->GetId()));
  f->MakePermanent();
  
  ns3::Ptr<ns3::ndn::fib::Entry> entry = ns3::CreateObject<ns3::ndn::fib::Entry>(this, prefix);
  entry->AddOrUpdateRoutingMetric(face, metric);
  return entry;
}

};//namespace ndnfd
