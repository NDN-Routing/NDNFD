#ifndef NDNFD_NS3_EXTRA_PRODUCER_THROTTLED_H_
#define NDNFD_NS3_EXTRA_PRODUCER_THROTTLED_H_
#include <unordered_map>
#include <ns3/ndnSIM/apps/ndn-producer.h>
#include <ns3/processing_delay.h>
#include <ns3/ndn-content-object.h>
namespace ns3 {
namespace ndn {

class ProducerThrottled : public Producer {
 public:
  static TypeId GetTypeId(void);
  ProducerThrottled(void);
  virtual ~ProducerThrottled(void) {}

  virtual void OnInterest(Ptr<const Interest> interest);

 private:
  Ptr<ProcessingDelay> pd_;
  uint32_t pending_i_;
  std::unordered_map<uint32_t,Ptr<ContentObject>> pendings_;
  
  Ptr<ProcessingDelay> GetProcessingDelay(void) const { return this->pd_; }
  void SetProcessingDelay(Ptr<ProcessingDelay> value);
  
  void ProcessComplete(ProcessingDelay::Job job);
};
NS_OBJECT_ENSURE_REGISTERED(ProducerThrottled);

};//namespace ndn
};//namespace ns3
#endif//NDNFD_NS3_EXTRA_PRODUCER_THROTTLED_H_
