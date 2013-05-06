#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>
#include <ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.h>

/*
One consumer and several producers are connected on one CsmaChannel.
*/

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();
  
  uint32_t n_producers = 4;
  uint32_t sim_time = 10;
  std::string frequency_str;
  ns3::CommandLine cmd;
  cmd.AddValue("n_producers", "number of producers", n_producers);
  cmd.AddValue("sim_time", "total simulation time (s)", sim_time);
  cmd.AddValue("frequency", "frequency | 'window'", frequency_str);
  cmd.Parse(argc, argv);

  ns3::Config::SetDefault("ns3::CsmaChannel::DataRate", ns3::StringValue("1Gbps"));
  ns3::Config::SetDefault("ns3::CsmaChannel::Delay", ns3::StringValue("6560ns"));
  ns3::Config::SetDefault("ns3::ProcessingDelay::NSlots", ns3::StringValue("1"));

  ns3::NodeContainer consumer_nodes; consumer_nodes.Create(1);
  ns3::NodeContainer producer_nodes; producer_nodes.Create(n_producers);
  ns3::NodeContainer nodes(consumer_nodes, producer_nodes);

  ns3::CsmaHelper csma;
  csma.Install(nodes);

  ndnfd::StackHelper ndnfdHelper;
  ndnfdHelper.Install(nodes);
  
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  if (frequency_str == "window") {
    consumerHelper = ns3::ndn::AppHelper("ns3::ndn::ConsumerWindow");
    consumerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  } else {
    consumerHelper.SetAttribute("Frequency", ns3::StringValue(frequency_str));
  }
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.Install(consumer_nodes);

  ns3::ndn::AppHelper producerHelper("ns3::ndn::ProducerThrottled");
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
  producerHelper.Install(producer_nodes);

  auto delay_tracers = ns3::ndn::AppDelayTracer::InstallAll("ndnfd-loadbal-csma_delay.tsv");

  ns3::Simulator::Stop(ns3::Seconds(sim_time));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
