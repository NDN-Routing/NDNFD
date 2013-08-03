#include "stack.h"
#include <ns3/simulator.h>
#include "../model/global.h"
#include "core/nameprefix_table.h"
#include "face/facemgr.h"
#include "face/dgram.h"
namespace ndnfd {

void StackHelper::WaitUntilMinStartTime(void) {
  ns3::Time diff = StackHelper::kMinStartTime() - ns3::Now();
  if (diff.IsNegative()) return;
  ns3::Simulator::Stop(diff);
  ns3::Simulator::Run();
}

StackHelper::StackHelper(void) : set_default_routes_(false) {}

void StackHelper::Install(ns3::Ptr<ns3::Node> node) const {
  NS_ASSERT_MSG(node->GetObject<ns3::ndn::L3Protocol>() == nullptr, "ndnSIM or NDNFD stack is already installed");
  NS_ASSERT_MSG(ns3::Now() >= StackHelper::kMinStartTime(), "cannot install before kMinStartTime");
  
  ns3::Ptr<L3Protocol> l3 = ns3::CreateObject<L3Protocol>();
  l3->Init(node);
  
  Ptr<NamePrefixTable> npt = l3->global()->npt();
  
  if (this->set_default_routes_) {
    Ptr<NamePrefixEntry> npe = npt->Seek(Name::FromUri("/"));
    for (auto tuple : l3->global()->facemgr()->ether_channels()) {
      Ptr<Face> face = std::get<2>(tuple);
      Ptr<ForwardingEntry> f = npe->SeekForwarding(face->id());
      f->native()->flags |= CCN_FORW_LAST;
      f->MakePermanent();
    }
  }
  
  for (auto pair : this->strategy_by_namespace_) {
    Ptr<NamePrefixEntry> npe = npt->Seek(Name::FromUri(pair.first));
    StrategyType strategy_type = StrategyType_Find(pair.second);
    NS_ASSERT_MSG(strategy_type != StrategyType_none, "strategy not known");
    npe->set_strategy_type(strategy_type);
  }
}

void StackHelper::Install(const ns3::NodeContainer& c) const {
  for (ns3::NodeContainer::Iterator it = c.Begin(); it != c.End(); ++it) {
    this->Install(*it);
  }
}

};//namespace ndnfd
