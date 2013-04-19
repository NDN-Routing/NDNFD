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

  ns3::Config::SetDefault("ns3::DropTailQueue::MaxPackets", ns3::StringValue("20"));
  ns3::Config::SetDefault("ns3::ndn::Consumer::RetxTimer", ns3::StringValue("300s"));
  
  ndnfd::SimBuildTopo(R"EOT(
    node A
    node B
    node C
    node D
    node E
    node F
    node G
    node H
    csma 1Mbps 10ms A B
    csma 1Mbps 10ms B C
    csma 1Mbps 10ms B H
    csma 1Mbps 10ms C D F
    csma 1Mbps 10ms D E
    csma 1Mbps 10ms F G
    csma 1Mbps 10ms E G
    ndnfd-stack
  )EOT");

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("5"));
  ns3::ApplicationContainer consumerA = consumerHelper.Install(ns3::Names::Find<ns3::Node>("A"));
  consumerA.Start(ns3::Seconds(0.0)); consumerA.Stop(ns3::Seconds(120.0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  ns3::ApplicationContainer producerD = producerHelper.Install(ns3::Names::Find<ns3::Node>("D"));
  producerD.Start(ns3::Seconds(0.0)); producerD.Stop(ns3::Seconds(30.0)); 
  ns3::ApplicationContainer producerE = producerHelper.Install(ns3::Names::Find<ns3::Node>("E"));
  producerE.Start(ns3::Seconds(29.9)); producerE.Stop(ns3::Seconds(60.0)); 
  ns3::ApplicationContainer producerF = producerHelper.Install(ns3::Names::Find<ns3::Node>("F"));
  producerF.Start(ns3::Seconds(59.9)); producerF.Stop(ns3::Seconds(90.0)); 
  ns3::ApplicationContainer producerG = producerHelper.Install(ns3::Names::Find<ns3::Node>("G"));
  producerG.Start(ns3::Seconds(89.9)); producerG.Stop(ns3::Seconds(110.0));
  ns3::ApplicationContainer producerH = producerHelper.Install(ns3::Names::Find<ns3::Node>("H"));
  producerH.Start(ns3::Seconds(0.0)); producerH.Stop(ns3::Seconds(10.0));

  ns3::Simulator::Stop(ns3::Seconds(122.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
