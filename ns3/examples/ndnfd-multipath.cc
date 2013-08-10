#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

/*
This example demostrates self-learning strategy can make use of multiple paths during high traffic,
and uses shorter paths during low traffic.
It only works when NDNFD is compiled with SelfLearnStrategy.

       ---C----D----E---
      /                 \
A----B                  F
      \                 /
       ---G---------H---

Producer F is always available.
Consumer A expresses 5 Interests/s, and 100 Interests/s during 30.0-60.0s.

*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  ns3::Config::SetDefault("ns3::ndn::Consumer::RetxTimer", ns3::StringValue("300s"));
  ns3::CommandLine cmd;
  cmd.Parse(argc, argv);
  
  ndnfd::SimBuildTopo(R"EOT(
    # http://www.ccnx.org/pipermail/ccnx-dev/2013-April/000974.html
    node A
    node B
    node C
    node D
    node E
    node F
    node G
    node H
    csma 900Kbps 6560ns A B
    csma 900Kbps 6560ns B C
    csma 900Kbps 6560ns B G
    csma 600Kbps 6560ns C D
    csma 600Kbps 6560ns D E
    csma 600Kbps 6560ns E F
    csma 600Kbps 6560ns G H
    csma 600Kbps 6560ns H F
    ndnfd-stack
  )EOT");

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("5"));
  consumerHelper.SetAttribute("StartSeq", ns3::IntegerValue(0));
  consumerHelper.SetAttribute("MaxSeq", ns3::IntegerValue(0+5*90));
  ns3::ApplicationContainer consumerA = consumerHelper.Install(ns3::Names::Find<ns3::Node>("A"));
  consumerA.Start(ns3::Seconds(0.0)); consumerA.Stop(ns3::Seconds(95.0));
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("95"));
  consumerHelper.SetAttribute("StartSeq", ns3::IntegerValue(1000));
  consumerHelper.SetAttribute("MaxSeq", ns3::IntegerValue(1000+95*30));
  ns3::ApplicationContainer consumerA2 = consumerHelper.Install(ns3::Names::Find<ns3::Node>("A"));
  consumerA2.Start(ns3::Seconds(30.0)); consumerA2.Stop(ns3::Seconds(65.0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  ns3::ApplicationContainer producerF = producerHelper.Install(ns3::Names::Find<ns3::Node>("F"));
  producerF.Start(ns3::Seconds(0.0)); producerF.Stop(ns3::Seconds(95.0));
  
  auto delay_tracers = ns3::ndn::AppDelayTracer::InstallAll("ndnfd-multipath_delay.tsv");
  ns3::Ptr<ndnfd::MessageCounter> message_counter = ns3::Create<ndnfd::MessageCounter>("ndnfd-multipath_msgcount.tsv");
  message_counter->ConnectAll();
  ns3::Ptr<ndnfd::MessageCounter> message_counterE = ns3::Create<ndnfd::MessageCounter>("ndnfd-multipath_msgcountE.tsv");
  message_counterE->ConnectNode(ns3::Names::Find<ns3::Node>("E"));
  ns3::Ptr<ndnfd::MessageCounter> message_counterH = ns3::Create<ndnfd::MessageCounter>("ndnfd-multipath_msgcountH.tsv");
  message_counterH->ConnectNode(ns3::Names::Find<ns3::Node>("H"));

  ns3::Simulator::Stop(ns3::Seconds(95.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
