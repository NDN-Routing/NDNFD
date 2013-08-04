#include "processing_delay.h"
#include <algorithm>
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
    .AddAttribute("QueueCapacity", "maximum number of queued jobs",
      UintegerValue(10),
      MakeUintegerAccessor(&ProcessingDelay::queue_capacity_),
      MakeUintegerChecker<size_t>(1))
    .AddAttribute("ProcessTime", "fixed processing time",
      TimeValue(MilliSeconds(1)),
      MakeTimeAccessor(&ProcessingDelay::SetProcessTime, &ProcessingDelay::GetProcessTime),
      MakeTimeChecker())
    .AddTraceSource("Drop", "drop a job because queue is full",
      MakeTraceSourceAccessor(&ProcessingDelay::drop_))
    ;
  return tid;
}

std::pair<std::vector<ProcessingDelay::SlotUsage>::iterator,std::vector<ProcessingDelay::SlotUsage>::iterator> ProcessingDelay::slot_range(void) {
  while (this->nslots_ > this->slots_.size()) {
    this->slots_.emplace_back(static_cast<uint32_t>(this->slots_.size()));
  }
  return std::make_pair(this->slots_.begin(), this->slots_.begin() + this->nslots_);
}

bool ProcessingDelay::CanStartImmediately(void) {
  auto slot_range = this->slot_range();
  return this->queue_.empty() && std::any_of(slot_range.first, slot_range.second, [] (const SlotUsage& slot) { return slot.idle_; });
}

void ProcessingDelay::SubmitJob(Job job) {
  if (this->queue_.size() >= this->queue_capacity_) {
    Ptr<AttributeValue> victim = this->queue_.front();
    this->queue_.pop();
    this->drop_(*(victim));
  }
  this->queue_.push(job.Copy());
  this->StartNext();
}

void ProcessingDelay::StartNext(void) {
  if (this->queue_.empty()) return;
  
  // find an idle slot
  auto slot_range = this->slot_range();
  auto slot_it = std::find_if(slot_range.first, slot_range.second, [] (const SlotUsage& slot) { return slot.idle_; });
  if (slot_it == slot_range.second) return;// no idle slot
  SlotUsage& slot = *slot_it;
  
  // place job on slot
  NS_ASSERT(slot.idle_);
  slot.idle_ = false;
  slot.job_ = this->queue_.front();
  this->queue_.pop();
  Time time = this->process_(*(slot.job_));
  slot.end_ = Simulator::Schedule(time, &ProcessingDelay::OnSlotEnd, this, slot.i_);
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
