#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/ndnSIM-module.h>
#include <ns3/NDNFD-module.h>

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndnfd-simple
 */

int main(int argc, char *argv[]) {
  ndnfd::StackHelper::WaitUntilMinStartTime();

  // setting default parameters for PointToPoint links and channels
  ns3::Config::SetDefault("ns3::PointToPointNetDevice::DataRate", ns3::StringValue("1Mbps"));
  ns3::Config::SetDefault("ns3::PointToPointChannel::Delay", ns3::StringValue("10ms"));
  ns3::Config::SetDefault("ns3::DropTailQueue::MaxPackets", ns3::StringValue("20"));

  // Creating nodes
  ns3::NodeContainer nodes;
  nodes.Create(3);

  // Connecting nodes using two links
  ns3::PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDNFD stack on all nodes
  ndnfd::StackHelper ndnfdHelper;
  ndnfdHelper.SetDefaultRoutes(true);
  ndnfdHelper.Install(nodes);

  // Installing applications

  // Consumer
  ns3::ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", ns3::StringValue("10")); // 10 interests a second
  consumerHelper.Install(nodes.Get(0)); // first node

  // Producer
  ns3::ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", ns3::StringValue("9999"));
  producerHelper.Install(nodes.Get(2)); // last node

  ns3::Simulator::Stop(ns3::Seconds(20.0));
  ns3::Simulator::Run();
  ns3::Simulator::Destroy();

  return 0;
}
