#ifndef NDNFD_NS3_HELPER_STACK_H_
#define NDNFD_NS3_HELPER_STACK_H_
#include <unordered_map>
#include <ns3/node-container.h>
#include <ns3/ndnfd-l3protocol.h>
namespace ndnfd {

class StackHelper {
 public:
  static ns3::Time kMinStartTime(void) { return L3Protocol::kMinStartTime(); }
  static void WaitUntilMinStartTime(void);
  
  StackHelper(void);
  virtual ~StackHelper(void) {}

  // If SetDefaultRoutes is true, a default route is added on each Ethernet multicast face.
  void SetDefaultRoutes(bool needSet) { this->set_default_routes_ = needSet; }
  
  // SetStrategy chooses a forwarding strategy (selected by its title) for a namespace (represented as URI).
  void SetStrategy(const std::string& name, const std::string& title) { this->strategy_by_namespace_[name] = title; }
  // ClearStrategy clears all strategy choices.
  void ClearStrategy(void) { this->strategy_by_namespace_.clear(); }
  
  // Install installs NDNFD on node.
  void Install(ns3::Ptr<ns3::Node> node) const;
  
  // Install installs NDNFD on each node in c.
  void Install(const ns3::NodeContainer& c) const;
  
 private:
  bool set_default_routes_;
  std::unordered_map<std::string,std::string> strategy_by_namespace_;
};

};//namespace ndnfd
#endif//NDNFD_NS3_HELPER_STACK_H_
