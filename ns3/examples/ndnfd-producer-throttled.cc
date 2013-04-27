#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>

/*
This scenario is a demo for ns3::ndn::ProducerThrottled

NS_LOG=ndn.Consumer:ndn.ProducerThrottled ./waf --run ndnfd-producer-throttled
*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  ns3::Config::SetDefault("ns3::ProcessingDelay::NSlots", ns3::StringValue("2"));
  ns3::Config::SetDefault("ns3::ProcessingDelay::ProcessTime", ns3::StringValue("210ms"));
  ns3::Config::SetDefault("ns3::ndn::Consumer::RetxTimer", ns3::StringValue("300s"));
  ns3::Config::SetDefault("ns3::ndn::ConsumerCbr::MaxSeq", ns3::StringValue("10"));

  ns3::NodeContainer nodes;
  nodes.Create(1);

  ndnfd::StackHelper ndnfdHelper;
  ndnfdHelper.Install(nodes);

  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("10"));
  consumerHelper.Install(nodes.Get(0));

  ns3::ndn::AppHelper producerHelper("ns3::ndn::ProducerThrottled");
  producerHelper.SetPrefix("/prefix");
  producerHelper.Install(nodes.Get(0));

  ns3::Simulator::Stop(ns3::Seconds(10.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
