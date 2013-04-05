#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>

/*
This example demostrates the ability of automatic failover in self-learning strategy.
It only works when NDNFD is compiled with SelfLearnStrategy.

               ---D----E
              /        |
A----B----C--+         |
     |        \        |
     H         ---F----G

Consumer A expresses 5 Interests per second for 0.0 ~ 120.0 seconds.
Producer D runs for  0.0 ~ 30.0 seconds.
Producer F runs for 29.9 ~ 60.0 seconds.
Producer E runs for 59.9 ~ 90.0 seconds.
Producer G runs for 89.9 ~110.0 seconds.
Producer H runs for  0.0 ~ 10.0 seconds.

*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  ns3::Config::SetDefault("ns3::CsmaChannel::DataRate", ns3::StringValue("1Mbps"));
  ns3::Config::SetDefault("ns3::CsmaChannel::Delay", ns3::StringValue("10ms"));
  ns3::Config::SetDefault("ns3::DropTailQueue::MaxPackets", ns3::StringValue("20"));
  ns3::Config::SetDefault("ns3::ndn::Consumer::RetxTimer", ns3::StringValue("300s"));

  ns3::NodeContainer nodes;
  nodes.Create(8);
  
  ns3::CsmaHelper csma;
  csma.Install(ns3::NodeContainer(nodes.Get(0), nodes.Get(1)));//A,B
  csma.Install(ns3::NodeContainer(nodes.Get(1), nodes.Get(2)));//B,C
  csma.Install(ns3::NodeContainer(nodes.Get(2), nodes.Get(3), nodes.Get(5)));//C,D,F
  csma.Install(ns3::NodeContainer(nodes.Get(3), nodes.Get(4)));//D,E
  csma.Install(ns3::NodeContainer(nodes.Get(5), nodes.Get(6)));//F,G
  csma.Install(ns3::NodeContainer(nodes.Get(4), nodes.Get(6)));//E,G
  csma.Install(ns3::NodeContainer(nodes.Get(1), nodes.Get(7)));//B,H

  ndnfd::StackHelper ndnfdHelper;
  //ndnfdHelper.SetDefaultRoutes(true);
  ndnfdHelper.Install(nodes);

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("5"));
  ns3::ApplicationContainer consumerA = consumerHelper.Install(nodes.Get(0));//A
  consumerA.Start(ns3::Seconds(0.0)); consumerA.Stop(ns3::Seconds(120.0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  ns3::ApplicationContainer producerD = producerHelper.Install(nodes.Get(3));
  producerD.Start(ns3::Seconds(0.0)); producerD.Stop(ns3::Seconds(30.0)); 
  ns3::ApplicationContainer producerE = producerHelper.Install(nodes.Get(4));
  producerE.Start(ns3::Seconds(29.9)); producerE.Stop(ns3::Seconds(60.0)); 
  ns3::ApplicationContainer producerF = producerHelper.Install(nodes.Get(5));
  producerF.Start(ns3::Seconds(59.9)); producerF.Stop(ns3::Seconds(90.0)); 
  ns3::ApplicationContainer producerG = producerHelper.Install(nodes.Get(6));
  producerG.Start(ns3::Seconds(89.9)); producerG.Stop(ns3::Seconds(110.0));
  ns3::ApplicationContainer producerH = producerHelper.Install(nodes.Get(7));
  producerH.Start(ns3::Seconds(0.0)); producerH.Stop(ns3::Seconds(10.0));

  ns3::Simulator::Stop(ns3::Seconds(122.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
