#ifndef NDNFD_NS3_UTILS_TRACING_H_
#define NDNFD_NS3_UTILS_TRACING_H_
#include <cstdio>
#include <ns3/event-id.h>
#include <ns3/node-container.h>
namespace ndnfd {

class L3Protocol;
class Name;

class Tracer : public ns3::SimpleRefCount<Tracer> {
 public:
  explicit Tracer(const std::string& filename);
  virtual ~Tracer(void);
  
  void ConnectAll(void) { this->Connect("*"); }
  void ConnectNode(ns3::Ptr<ns3::Node> node);

  void PrintHeader(void);
  void Print(void);
 
 private:
  FILE* outfile_;
  uintmax_t total_mcast_send_;
  uintmax_t total_mcast_recv_;
  uintmax_t total_unicast_send_;
  uintmax_t recent_mcast_send_;
  uintmax_t recent_mcast_recv_;
  uintmax_t recent_unicast_send_;

  ns3::EventId next_print_;
  
  void PeriodicPrinter(void);

  void Connect(const std::string& node_selector);
  void InterestMcastSend(ns3::Ptr<L3Protocol> l3, const Name* name);
  void InterestMcastRecv(ns3::Ptr<L3Protocol> l3, const Name* name);
  void InterestUnicastSend(ns3::Ptr<L3Protocol> l3, const Name* name);
};

};//namespace ndnfd
#endif//NDNFD_NS3_UTILS_TRACING_H_
