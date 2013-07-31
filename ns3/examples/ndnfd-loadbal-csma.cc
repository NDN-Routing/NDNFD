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
  
  ns3::Config::SetDefault("ns3::CsmaChannel::DataRate", ns3::StringValue("1Gbps"));
  ns3::Config::SetDefault("ns3::CsmaChannel::Delay", ns3::StringValue("6560ns"));
  ns3::Config::SetDefault("ns3::ProcessingDelay::NSlots", ns3::StringValue("1"));

  uint32_t n_producers = 4;
  uint32_t n_consumers = 1;
  uint32_t maxseq = 4096;
  uint32_t sim_time = 10;
  std::string frequency_str("15.0");
  ns3::CommandLine cmd;
  cmd.AddValue("n_producers", "number of producers", n_producers);
  cmd.AddValue("n_consumers", "number of consumers", n_consumers);
  cmd.AddValue("maxseq", "count of Interests per consumer", maxseq);
  cmd.AddValue("sim_time", "total simulation time (s)", sim_time);
  cmd.AddValue("frequency", "frequency | 'window'", frequency_str);
  cmd.Parse(argc, argv);

  ns3::NodeContainer producer_nodes; producer_nodes.Create(n_producers);
  ns3::NodeContainer consumer_nodes; consumer_nodes.Create(n_consumers);
  ns3::NodeContainer nodes(producer_nodes, consumer_nodes);

  ns3::CsmaHelper csma;
  csma.Install(nodes);

  ndnfd::StackHelper ndnfdHelper;
  ndnfdHelper.Install(nodes);
  
  if (frequency_str == "window") {
    ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerWindow");
    consumerHelper.SetAttribute("PayloadSize", ns3::StringValue("1024"));
    consumerHelper.SetPrefix("/prefix");
    for (uint32_t i = 0; i < n_consumers; ++i) {
      consumerHelper.SetAttribute("StartSeq", ns3::IntegerValue(1000000*i));
      consumerHelper.SetAttribute("Size", ns3::DoubleValue((1000000*i + maxseq) / 1024.0));
      consumerHelper.Install(consumer_nodes.Get(i));
    }
  } else {
    ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
    consumerHelper.SetAttribute("Frequency", ns3::StringValue(frequency_str));
    consumerHelper.SetPrefix("/prefix");
    for (uint32_t i = 0; i < n_consumers; ++i) {
      consumerHelper.SetAttribute("StartSeq", ns3::IntegerValue(1000000*i));
      consumerHelper.SetAttribute("MaxSeq", ns3::IntegerValue(1000000*i + maxseq));
      consumerHelper.Install(consumer_nodes.Get(i));
    }
  }

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
