#include "stack.h"
#include <ns3/simulator.h>
#include "../model/global.h"
#include "core/nameprefix_table.h"
#include "face/facemgr.h"
#include "face/dgram.h"
namespace ndnfd {

StackHelper::StackHelper(void) : set_default_routes_(false) {}

void StackHelper::Install(ns3::Ptr<ns3::Node> node) const {
  NS_ASSERT_MSG(node->GetObject<ns3::ndn::L3Protocol>() == nullptr, "ndnSIM or NDNFD stack is already installed");
  NS_ASSERT_MSG(ns3::Now() >= StackHelper::kMinStartTime(), "cannot install before kMinStartTime");
  
  ns3::Ptr<L3Protocol> l3 = ns3::CreateObject<L3Protocol>();
  l3->Init(node);
  
  if (this->set_default_routes_) {
    Ptr<Name> root_name = Name::FromUri("/");
    Ptr<NamePrefixEntry> npe = l3->global()->npt()->Seek(root_name);
    for (auto tuple : l3->global()->facemgr()->ether_channels()) {
      Ptr<Face> face = std::get<2>(tuple);
      Ptr<ForwardingEntry> f = npe->SeekForwarding(face->id());
      f->forw()->flags |= CCN_FORW_LAST;
      f->MakePermanent();
    }
  }
}

void StackHelper::Install(const ns3::NodeContainer& c) const {
  for (ns3::NodeContainer::Iterator it = c.Begin(); it != c.End(); ++it) {
    this->Install(*it);
  }
}

};//namespace ndnfd
