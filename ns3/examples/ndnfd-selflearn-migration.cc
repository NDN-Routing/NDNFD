#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

/*
This example demostrates self-learning strategy can adapt to server migration.
It only works when NDNFD is compiled with SelfLearnStrategy.

               ---D----E
              /        |
A----B----C--+         |
     |        \        |
     H         ---F----G

Consumer A expresses 50 Interests per second during 0 ~ 120 seconds.
Producer H runs for  0 ~ 10 and 90 ~ 110 seconds.
Producer D runs for  0 ~ 30 seconds.
Producer E runs for 30 ~ 60 seconds.
Producer F runs for 60 ~ 90 seconds.

*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  //ns3::Config::SetDefault("ns3::DropTailQueue::MaxPackets", ns3::StringValue("20"));
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
    csma 10Mbps 1ms A B
    csma 10Mbps 1ms B C
    csma 10Mbps 1ms B H
    csma 10Mbps 1ms C D F
    csma 10Mbps 1ms D E
    csma 10Mbps 1ms F G
    csma 10Mbps 1ms E G
    ndnfd-stack #default-route
  )EOT");

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("50"));
  ns3::ApplicationContainer consumerA = consumerHelper.Install(ns3::Names::Find<ns3::Node>("A"));
  consumerA.Start(ns3::Seconds(0.0)); consumerA.Stop(ns3::Seconds(120.0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  ns3::ApplicationContainer producerH = producerHelper.Install(ns3::Names::Find<ns3::Node>("H"));
  producerH.Start(ns3::Seconds(0.0)); producerH.Stop(ns3::Seconds(10.0));
  ns3::ApplicationContainer producerD = producerHelper.Install(ns3::Names::Find<ns3::Node>("D"));
  producerD.Start(ns3::Seconds(10.0)); producerD.Stop(ns3::Seconds(30.0)); 
  ns3::ApplicationContainer producerE = producerHelper.Install(ns3::Names::Find<ns3::Node>("E"));
  producerE.Start(ns3::Seconds(30.0)); producerE.Stop(ns3::Seconds(60.0)); 
  ns3::ApplicationContainer producerF = producerHelper.Install(ns3::Names::Find<ns3::Node>("F"));
  producerF.Start(ns3::Seconds(60.0)); producerF.Stop(ns3::Seconds(90.0)); 
  ns3::ApplicationContainer producerH2 = producerHelper.Install(ns3::Names::Find<ns3::Node>("H"));
  producerH2.Start(ns3::Seconds(90.0)); producerH2.Stop(ns3::Seconds(110.0));
  
  auto delay_tracers = ns3::ndn::AppDelayTracer::InstallAll("ndnfd-selflearn-migration_delay.tsv");
  ns3::Ptr<ndnfd::Tracer> l3_tracer = ns3::Create<ndnfd::Tracer>("ndnfd-selflearn-migration_l3.tsv");
  l3_tracer->ConnectAll();

  ns3::Simulator::Stop(ns3::Seconds(122.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
