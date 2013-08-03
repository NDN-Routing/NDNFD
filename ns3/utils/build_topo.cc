#include "build_topo.h"
#include <sstream>
#include <ns3/names.h>
#include <ns3/string.h>
#include <ns3/csma-helper.h>
#include <ns3/point-to-point-helper.h>
#include <ns3/internet-stack-helper.h>
#include <ns3/ipv4-address-helper.h>
#include <ns3/ipv4-global-routing-helper.h>
#include <ns3/ndnfd-stack-helper.h>
namespace ndnfd {

std::tuple<ns3::NodeContainer,std::vector<ns3::NetDeviceContainer>> SimBuildTopo(std::string topo_desc, std::string name_path) {
  ns3::NodeContainer nodes; std::vector<ns3::NetDeviceContainer> links;
  
  std::istringstream input(topo_desc);
  
  char line[1024];
  while (input.good()) {
    input.getline(line, sizeof(line));
    if (line[0] == '\0') continue;

    std::vector<std::string> tokens;
    char* token = strtok(line, " \t\r\n");
    while (token != nullptr && token[0] != '#') {
      tokens.push_back(token);
      token = strtok(nullptr, " \t\r\n");
    }
    if (tokens.size() == 0) continue;
    
    if (tokens[0] == "node") {
      assert(tokens.size() == 2);
      nodes.Create(1);
      ns3::Ptr<ns3::Node> node = nodes.Get(nodes.GetN() - 1);
      ns3::Names::Add(name_path, tokens[1], node);
    } else if (tokens[0] == "csma") {
      assert(tokens.size() >= 4);
      ns3::CsmaHelper csma;
      csma.SetChannelAttribute("DataRate", ns3::StringValue(tokens[1]));
      csma.SetChannelAttribute("Delay", ns3::StringValue(tokens[2]));
      ns3::NodeContainer group;
      for (auto it = tokens.cbegin()+3; it != tokens.end(); ++it) {
        ns3::Ptr<ns3::Node> node = ns3::Names::Find<ns3::Node>(name_path, *it);
        assert(node != nullptr);
        group.Add(node);
      }
      links.push_back(csma.Install(group));
    } else if (tokens[0] == "p2p") {
      assert(tokens.size() == 5);
      ns3::PointToPointHelper p2p;
      p2p.SetDeviceAttribute("DataRate", ns3::StringValue(tokens[1]));
      p2p.SetChannelAttribute("Delay", ns3::StringValue(tokens[2]));
      ns3::Ptr<ns3::Node> node1 = ns3::Names::Find<ns3::Node>(name_path, tokens[3]);
      assert(node1 != nullptr);
      ns3::Ptr<ns3::Node> node2 = ns3::Names::Find<ns3::Node>(name_path, tokens[4]);
      assert(node2 != nullptr);
      links.push_back(p2p.Install(node1, node2));
    } else if (tokens[0] == "ip-stack") {
      bool global_route = false;
      for (auto it = tokens.cbegin()+1; it != tokens.end(); ++it) {
        if (*it == "global-route") global_route = true;
      }
      ns3::InternetStackHelper ipstack; ipstack.Install(nodes);
      ns3::Ipv4AddressHelper ipv4addr(ns3::Ipv4Address("10.0.0.0"), ns3::Ipv4Mask("255.255.255.0"));
      for (auto it = links.begin(); it != links.end(); ++it) {
        ipv4addr.Assign(*it);
        ipv4addr.NewNetwork();
      }
      if (global_route) {
        ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables();
      }
    } else if (tokens[0] == "ndnfd-stack") {
      ndnfd::StackHelper ndnfdstack;
      for (auto it = tokens.cbegin()+1; it != tokens.end(); ++it) {
        const std::string& token = *it;
        if (token == "default-route") {
          ndnfdstack.SetDefaultRoutes(true);
        } else if (token.substr(0, 9) == "strategy[") {
          size_t pos = token.rfind("]=");
          if (pos != std::string::npos) {
            ndnfdstack.SetStrategy(token.substr(9, pos-9), token.substr(pos+2));
          }
        } else if (token.substr(0, 9) == "strategy=") {
          ndnfdstack.SetStrategy("/", token.substr(9));
        }
      }
      ndnfdstack.Install(nodes);
    } else {
      assert(false);
    }
  }
  
  return std::forward_as_tuple(nodes, links);
}

};//namespace ndnfd
