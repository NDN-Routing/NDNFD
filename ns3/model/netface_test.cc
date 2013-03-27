#include <ns3/csma-helper.h>
#include "netface.h"
#include "ndnfdsim.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(SimTest, SimNetChannel) {
  EXPECT_EXIT({
    ns3::NodeContainer nodes;
    nodes.Create(3);
    ns3::CsmaHelper csma;
    ns3::NetDeviceContainer devs = csma.Install(nodes);
    assert(devs.GetN() == 3);
    
    NetworkAddress addrs[3];
    for (uint32_t i = 0; i < 3; ++i) {
      ns3::Ptr<ns3::NetDevice> dev = devs.Get(i);
      addrs[i] = SimNetChannel::ConvertAddress(dev->GetAddress());
    }
    
    for (uint32_t i = 0; i < 3; ++i) {
      ns3::Ptr<ns3::NetDevice> dev = devs.Get(i);
      ns3::Ptr<ns3::Node> node = dev->GetNode();
      ns3::Ptr<L3Protocol> l3 = ns3::CreateObject<L3Protocol>();
      ns3::Simulator::Schedule(L3Protocol::kMinStartTime(), &L3Protocol::Init, l3, node);
    }
    
    ns3::Simulator::Stop(ns3::Seconds(20));
    ns3::Simulator::Run();

    exit(0);
  }, ::testing::ExitedWithCode(0), "");
}

};//namespace ndnfd
