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

Consumer A expresses 2 Interests per second for 0.0 ~ 12.0 seconds.
Producer D runs for  0.0 ~ 3.0 seconds.
Producer F runs for  2.9 ~ 6.0 seconds.
Producer E runs for  5.9 ~ 9.0 seconds.
Producer G runs for  8.9 ~11.0 seconds.
Producer H runs for  0.0 ~ 1.0 seconds.

*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  ns3::Config::SetDefault("ns3::CsmaChannel::DataRate", ns3::StringValue("1Mbps"));
  ns3::Config::SetDefault("ns3::CsmaChannel::Delay", ns3::StringValue("10ms"));
  ns3::Config::SetDefault("ns3::DropTailQueue::MaxPackets", ns3::StringValue("20"));
  ns3::Config::SetDefault("ns3::ndn::Consumer::RetxTimer", ns3::StringValue("60s"));

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
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("2"));
  ns3::ApplicationContainer consumerA = consumerHelper.Install(nodes.Get(0));//A
  consumerA.Start(ns3::Seconds(0.0)); consumerA.Stop(ns3::Seconds(12.0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  ns3::ApplicationContainer producerD = producerHelper.Install(nodes.Get(3));
  producerD.Start(ns3::Seconds(0.0)); producerD.Stop(ns3::Seconds(3.0)); 
  ns3::ApplicationContainer producerE = producerHelper.Install(nodes.Get(4));
  producerE.Start(ns3::Seconds(2.9)); producerE.Stop(ns3::Seconds(6.0)); 
  ns3::ApplicationContainer producerF = producerHelper.Install(nodes.Get(5));
  producerF.Start(ns3::Seconds(5.9)); producerF.Stop(ns3::Seconds(9.0)); 
  ns3::ApplicationContainer producerG = producerHelper.Install(nodes.Get(6));
  producerG.Start(ns3::Seconds(8.9)); producerG.Stop(ns3::Seconds(11.0));
  ns3::ApplicationContainer producerH = producerHelper.Install(nodes.Get(7));
  producerH.Start(ns3::Seconds(0.0)); producerH.Stop(ns3::Seconds(1.0));

  ns3::Simulator::Stop(ns3::Seconds(12.5));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
