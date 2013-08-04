#ifndef NDNFD_NS3_UTILS_MESSAGE_COUNTER_H_
#define NDNFD_NS3_UTILS_MESSAGE_COUNTER_H_
#include <cstdio>
#include <ns3/event-id.h>
#include <ns3/node-container.h>
namespace ndnfd {

class L3Protocol;
typedef uint16_t MessageType;

class MessageCounter : public ns3::SimpleRefCount<MessageCounter> {
 public:
  explicit MessageCounter(const std::string& filename);
  virtual ~MessageCounter(void);
  
  void ConnectAll(void) { this->Connect("*"); }
  void ConnectNode(ns3::Ptr<ns3::Node> node);

  void PrintHeader(void);
  void Print(void);
 
 private:
  FILE* outfile_;
  
  // counters: Send|Recv, Mcast|Unicast, Interest|ContentObject|Nack
  uintmax_t SMI_;
  uintmax_t SMC_;
  uintmax_t SMN_;
  uintmax_t SUI_;
  uintmax_t SUC_;
  uintmax_t SUN_;
  uintmax_t RMI_;
  uintmax_t RMC_;
  uintmax_t RMN_;
  uintmax_t RUI_;
  uintmax_t RUC_;
  uintmax_t RUN_;

  ns3::EventId next_print_;
  
  void PeriodicPrinter(void);

  void Connect(const std::string& node_selector);

  void OnSend(ns3::Ptr<L3Protocol> l3, MessageType t, bool is_mcast);
  void OnRecv(ns3::Ptr<L3Protocol> l3, MessageType t, bool is_mcast);
};

};//namespace ndnfd
#endif//NDNFD_NS3_UTILS_MESSAGE_COUNTER_H_
