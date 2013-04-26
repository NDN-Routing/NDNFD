#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>
#include <ns3/ndnSIM/utils/tracers/l2-rate-tracer.h>

/*
This example demostrates self-learning strategy can make use of multiple paths during high traffic,
and uses shorter paths during low traffic.
It only works when NDNFD is compiled with SelfLearnStrategy.

          ---C----D----E---
         /                 \
A----B--+                  F
         \                 /
          ---G---------H---

Producer F is always available.
Consumer A expresses 5 Interests/s, and 50 Interests/s during 30.0-60.0s.

*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  ns3::Config::SetDefault("ns3::DropTailQueue::MaxPackets", ns3::StringValue("20"));
  ns3::Config::SetDefault("ns3::ndn::Consumer::RetxTimer", ns3::StringValue("300s"));
  
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
    csma 900Kbps 6560ns B C G
    csma 600Kbps 6560ns C D
    csma 600Kbps 6560ns D E
    csma 600Kbps 6560ns E F
    csma 300Kbps 6560ns G H
    csma 300Kbps 6560ns H F
    ndnfd-stack
  )EOT");

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("5"));
  ns3::ApplicationContainer consumerA = consumerHelper.Install(ns3::Names::Find<ns3::Node>("A"));
  consumerA.Start(ns3::Seconds(0.0)); consumerA.Stop(ns3::Seconds(90.0));
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("45"));
  consumerHelper.SetAttribute("StartSeq", ns3::StringValue("1000000000"));
  ns3::ApplicationContainer consumerA2 = consumerHelper.Install(ns3::Names::Find<ns3::Node>("A"));
  consumerA2.Start(ns3::Seconds(30.0)); consumerA2.Stop(ns3::Seconds(60.0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  ns3::ApplicationContainer producerF = producerHelper.Install(ns3::Names::Find<ns3::Node>("F"));
  producerF.Start(ns3::Seconds(0.0)); producerF.Stop(ns3::Seconds(90.0)); 
  
  auto delay_tracers = ns3::ndn::AppDelayTracer::InstallAll("ndnfd-selflearn-multipath_delay.tsv");
  ns3::Ptr<ndnfd::Tracer> l3_tracer = ns3::Create<ndnfd::Tracer>("ndnfd-selflearn-multipath_l3.tsv");
  l3_tracer->ConnectAll();
  ns3::Ptr<ndnfd::Tracer> l3_tracerE = ns3::Create<ndnfd::Tracer>("ndnfd-selflearn-multipath_l3_E.tsv");
  l3_tracerE->ConnectNode(ns3::Names::Find<ns3::Node>("E"));
  ns3::Ptr<ndnfd::Tracer> l3_tracerH = ns3::Create<ndnfd::Tracer>("ndnfd-selflearn-multipath_l3_H.tsv");
  l3_tracerH->ConnectNode(ns3::Names::Find<ns3::Node>("H"));

  ns3::Simulator::Stop(ns3::Seconds(92.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
