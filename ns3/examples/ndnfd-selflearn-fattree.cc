#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>
#include <ns3/ndnSIM/utils/tracers/l2-rate-tracer.h>

/*
Topology: fat tree of 16 end hosts organized into three pods.

   C0      C1      C2      C3   core
  / ______/       /       /
 | |  ___________/       /
 | | | _________________/
 |/  |/ (A*0 connect to C0 C1, A*1 connect to C2 C3)
A00 A01 A10 A11 A20 A21 A30 A31  aggreation
 | X |   | X |   | X |   | X | 
E00 E01 E10 E11 E20 E21 E30 E31  edge
 |   |   |   |   |   |   |   |  
H00 H02 H10 H12 H20 H22 H30 H32  end hosts
H01 H03 H11 H13 H21 H23 H31 H33


Simulation runs for 20 seconds.
Every end host expresses 2 Interests per second to each end host.


*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  ns3::Config::SetDefault("ns3::DropTailQueue::MaxPackets", ns3::StringValue("20"));
  ns3::Config::SetDefault("ns3::ndn::Consumer::RetxTimer", ns3::StringValue("300s"));
  
  ndnfd::SimBuildTopo(R"EOT(
    # core routers
    node C0
    node C1
    node C2
    node C3
    # aggregation routers
    node A00
    node A01
    node A10
    node A11
    node A20
    node A21
    node A30
    node A31
    # edge routers
    node E00
    node E01
    node E10
    node E11
    node E20
    node E21
    node E30
    node E31
    # hosts
    node H00
    node H01
    node H02
    node H03
    node H10
    node H11
    node H12
    node H13
    node H20
    node H21
    node H22
    node H23
    node H30
    node H31
    node H32
    node H33
    # core - aggregation links
    csma 5Mbps 5ms C0 A00
    csma 5Mbps 5ms C0 A10
    csma 5Mbps 5ms C0 A20
    csma 5Mbps 5ms C0 A30
    csma 5Mbps 5ms C1 A00
    csma 5Mbps 5ms C1 A10
    csma 5Mbps 5ms C1 A20
    csma 5Mbps 5ms C1 A30
    csma 5Mbps 5ms C2 A01
    csma 5Mbps 5ms C2 A11
    csma 5Mbps 5ms C2 A21
    csma 5Mbps 5ms C2 A31
    csma 5Mbps 5ms C3 A01
    csma 5Mbps 5ms C3 A11
    csma 5Mbps 5ms C3 A21
    csma 5Mbps 5ms C3 A31
    # aggregation - edge links
    csma 5Mbps 5ms A00 E00
    csma 5Mbps 5ms A00 E01
    csma 5Mbps 5ms A01 E00
    csma 5Mbps 5ms A01 E01
    csma 5Mbps 5ms A10 E10
    csma 5Mbps 5ms A10 E11
    csma 5Mbps 5ms A11 E10
    csma 5Mbps 5ms A11 E11
    csma 5Mbps 5ms A20 E20
    csma 5Mbps 5ms A20 E21
    csma 5Mbps 5ms A21 E20
    csma 5Mbps 5ms A21 E21
    csma 5Mbps 5ms A30 E30
    csma 5Mbps 5ms A30 E31
    csma 5Mbps 5ms A31 E30
    csma 5Mbps 5ms A31 E31
    # edge - host links
    csma 5Mbps 5ms E00 H00
    csma 5Mbps 5ms E00 H01
    csma 5Mbps 5ms E01 H02
    csma 5Mbps 5ms E01 H03
    csma 5Mbps 5ms E10 H10
    csma 5Mbps 5ms E10 H11
    csma 5Mbps 5ms E11 H12
    csma 5Mbps 5ms E11 H13
    csma 5Mbps 5ms E20 H20
    csma 5Mbps 5ms E20 H21
    csma 5Mbps 5ms E21 H22
    csma 5Mbps 5ms E21 H23
    csma 5Mbps 5ms E30 H30
    csma 5Mbps 5ms E30 H31
    csma 5Mbps 5ms E31 H32
    csma 5Mbps 5ms E31 H33
    # stack
    ndnfd-stack
  )EOT");
  
  std::vector<std::string> hostnames = { "H00", "H01", "H02", "H03", "H10", "H11", "H12", "H13", "H20", "H21", "H22", "H23", "H30", "H31", "H32", "H33" };

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("2"));
  ns3::ApplicationContainer consumers;
  for (auto consumer_host : hostnames) {
    for (auto producer_host : hostnames) {
      consumerHelper.SetAttribute("StartSeq", ns3::StringValue(producer_host.substr(1) + "000000"));
      consumerHelper.SetPrefix("/" + producer_host);
      consumers.Add(consumerHelper.Install(ns3::Names::Find<ns3::Node>(consumer_host)));
    }
  }
  consumers.Start(ns3::Seconds(0.0)); consumers.Stop(ns3::Seconds(20.0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  ns3::ApplicationContainer producers;
  for (auto producer_host : hostnames) {
    producerHelper.SetPrefix("/" + producer_host);
    producers.Add(producerHelper.Install(ns3::Names::Find<ns3::Node>(producer_host)));
  }
  producerHelper.SetPrefix("/prefix");
  producers.Start(ns3::Seconds(0.0)); producers.Stop(ns3::Seconds(20.0)); 
  
  auto delay_tracers = ns3::ndn::AppDelayTracer::InstallAll("ndnfd-selflearn-fattree_delay.tsv");
  ns3::Ptr<ndnfd::Tracer> l3_tracer = ns3::Create<ndnfd::Tracer>("ndnfd-selflearn-fattree_l3.tsv");
  l3_tracer->ConnectAll();

  ns3::Simulator::Stop(ns3::Seconds(22.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
