#include "processing_delay.h"
#include <ns3/uinteger.h>
namespace ns3 {

TypeId ProcessingDelay::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::ProcessingDelay")
    .SetParent<Object>()
    .AddConstructor<ProcessingDelay>()
    .AddAttribute("NSlots", "number of processing slots",
      UintegerValue(1),
      MakeUintegerAccessor(&ProcessingDelay::nslots_),
      MakeUintegerChecker<uint32_t>(1))
    .AddAttribute("ProcessTime", "fixed processing time",
      TimeValue(MilliSeconds(1)),
      MakeTimeAccessor(&ProcessingDelay::SetProcessTime, &ProcessingDelay::GetProcessTime),
      MakeTimeChecker())
    ;
  return tid;
}

void ProcessingDelay::SubmitJob(Job job, Priority priority) {
  this->queue_.push(std::make_tuple(job.Copy(), priority));
  this->StartNext();
}

void ProcessingDelay::StartNext(void) {
  if (this->queue_.empty()) return;
  
  while (this->nslots_ > this->slots_.size()) {
    this->slots_.emplace_back(static_cast<uint32_t>(this->slots_.size()));
  }
  
  // find an idle slot
  uint32_t i;
  for (i = 0; i < this->nslots_; ++i) {
    if (this->slots_[i].idle_) break;
  }
  if (i >= this->nslots_) return;
  SlotUsage& slot = this->slots_[i];
  
  // place job on slot
  NS_ASSERT(slot.idle_);
  slot.idle_ = false;
  slot.job_ = std::get<0>(this->queue_.top());
  this->queue_.pop();
  Time time = this->process_(*(slot.job_));
  slot.end_ = Simulator::Schedule(time, &ProcessingDelay::OnSlotEnd, this, i);
}

void ProcessingDelay::SetProcessTime(Time time) {
  NS_ASSERT(time.IsPositive());
  this->process_time_ = time;
  this->process_ = MakeCallback(&ProcessingDelay::FixedTimeProcess, this);
}

Time ProcessingDelay::FixedTimeProcess(Job job) const {
  NS_ASSERT(this->process_time_.IsPositive());
  return this->process_time_;
}

void ProcessingDelay::SetProcessCallback(Callback<Time,Job> cb) {
  NS_ASSERT(!cb.IsNull());
  this->process_time_ = Time(-1);
  this->process_ = cb;
}

void ProcessingDelay::OnSlotEnd(uint32_t i) {
  SlotUsage& slot = this->slots_[i];
  NS_ASSERT(!slot.idle_);
  if (!this->complete_.IsNull()) this->complete_(*(slot.job_));
  slot.idle_ = true;
  
  this->StartNext();
}

};//namespace ndnfd
