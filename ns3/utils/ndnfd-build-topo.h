#ifndef NDNFD_NS3_UTILS_BUILD_TOPO_H_
#define NDNFD_NS3_UTILS_BUILD_TOPO_H_
#include "util/defs.h"
#include <ns3/node-container.h>
#include <ns3/net-device-container.h>
namespace ndnfd {

// SimBuildTopo builds a ns-3 topology.
std::tuple<ns3::NodeContainer,std::vector<ns3::NetDeviceContainer>> SimBuildTopo(std::string topo_desc, std::string name_path = "");

/*

# TOPOLOGY FILE FORMAT
# empty lines, or lines starting with # are comments

# node <nodename>
node r1 # comment
node r2
node r3
node r4

# csma data-rate delay node1 nodes2 ..
csma 1Mbps 10ms r1 r2 r3

# p2p data-rate delay node1 node2
p2p 1Mbps 10ms r3 r4

# ip-stack option1 ..
ip-stack global-route

# ndnfd-stack option1 ..
ndnfd-stack default-route strategy[/ndn/broadcast]=bcast strategy=original

*/


};//namespace ndnfd
#endif//NDNFD_NS3_UTILS_BUILD_TOPO_H_
