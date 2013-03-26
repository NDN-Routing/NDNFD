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
    
    SimGlobal* globals[3];
    Ptr<NdnfdSim> programs[3];
    for (uint32_t i = 0; i < 3; ++i) {
      ns3::Ptr<ns3::NetDevice> dev = devs.Get(i);
      ns3::Ptr<ns3::Node> node = dev->GetNode();
      SimGlobal* global = globals[i] = new SimGlobal();
      global->Init();
      ns3::Ptr<L3Protocol> l3 = ns3::CreateObject<L3Protocol>();
      global->set_l3(l3);
      l3->AggregateNode(node);
      Ptr<NdnfdSim> program = programs[i] = Element::MakeFirstElement(global)->New<NdnfdSim>(node->GetId());
      program->Start();
    }
    
    ns3::Simulator::Stop(ns3::Seconds(5));
    ns3::Simulator::Run();

    exit(0);
  }, ::testing::ExitedWithCode(0), "");
}

};//namespace ndnfd
