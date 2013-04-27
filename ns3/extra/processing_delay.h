#ifndef NDNFD_NS3_EXTRA_PROCESSING_DELAY_H_
#define NDNFD_NS3_EXTRA_PROCESSING_DELAY_H_
#include <queue>
#include <tuple>
#include <ns3/simulator.h>
namespace ns3 {

// ProcessingDelay models the processing delay in an application
// with limited processing slots.
// Each slot is able to handle one job at the same time.
class ProcessingDelay : public Object {
 public:
  typedef const AttributeValue& Job;
  typedef uint8_t Priority;
  
  static TypeId GetTypeId(void);
  ProcessingDelay(void) {}
  virtual ~ProcessingDelay(void) {}

  // SubmitJob submits a job.
  void SubmitJob(Job job, Priority priority);
  
  // Process callback determines the processing time for a job.
  void SetProcessCallback(Callback<Time,Job> cb);
  
  // Complete callback is invoked when the processing for a job is finished.
  void SetCompleteCallback(Callback<void,Job> cb) { this->complete_ = cb; }

 private:
  struct SlotUsage {
    SlotUsage(uint32_t i) : i_(i), idle_(true), job_(nullptr) {}
    ~SlotUsage(void) { this->end_.Cancel(); }
    uint32_t i_;
    bool idle_;
    Ptr<AttributeValue> job_;
    EventId end_;
  };
  typedef std::tuple<Ptr<AttributeValue>,Priority> JobItem;
  class JobItemComparer {
   public:
    bool operator()(const JobItem& lhs, const JobItem& rhs) const { return std::get<1>(lhs) > std::get<1>(rhs); }
  };
  
  uint32_t nslots_;
  std::vector<SlotUsage> slots_;
  std::priority_queue<JobItem,std::vector<JobItem>,JobItemComparer> queue_;
  Time process_time_;
  Callback<Time,Job> process_;
  Callback<void,Job> complete_;

  Time GetProcessTime(void) const { return this->process_time_; }
  void SetProcessTime(Time time);
  Time FixedTimeProcess(Job job) const;
  
  void OnSlotEnd(uint32_t i);
  void StartNext(void);
};
NS_OBJECT_ENSURE_REGISTERED(ProcessingDelay);

};//namespace ns3
#endif//NDNFD_NS3_EXTRA_PROCESSING_DELAY_H_
