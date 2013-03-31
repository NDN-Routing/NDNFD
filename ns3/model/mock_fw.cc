#include "mock_fw.h"
namespace ndnfd {

ns3::TypeId MockForwardingStrategy::GetTypeId(void) {
  static ns3::TypeId tid = ns3::TypeId("ndnfd::MockForwardingStrategy")
    .SetGroupName("NDNFD")
    .SetParent<ns3::ndn::ForwardingStrategy>()
    .AddConstructor<MockForwardingStrategy>();
  return tid;
}

};//namespace ndnfd
