#include "scheduler.h"
extern "C" {
void ccnd_gettime(const struct ccn_gettime *self, struct ccn_timeval *result);
}
namespace ndnfd {

constexpr std::chrono::microseconds Scheduler::kNoMore;

Scheduler::Scheduler(void) {
  this->sched_ = nullptr;
}

Scheduler::Scheduler(ccn_schedule* sched) {
  assert(sched != nullptr);
  this->sched_ = sched;
}

Scheduler::~Scheduler(void) {
  if (this->sched_ != nullptr) {
    ccn_schedule_destroy(&this->sched_);
  }
}

SchedulerEvent Scheduler::Schedule(std::chrono::microseconds delay, Callback cb, SchedulerEvent* evt_ptr, bool cancel_old) {
  assert(cb != nullptr);
  if (this->sched() == nullptr) {
    this->Log(kLLError, kLCScheduler, "Scheduler::Schedule sched is null");
    if (evt_ptr != nullptr) *evt_ptr = nullptr;
    return nullptr;
  }

  if (cancel_old) {
    assert(evt_ptr != nullptr);
    this->Cancel(*evt_ptr);
  }
  
  EvData* evdata = new EvData();
  evdata->scheduler = this;
  evdata->cb = cb;
  evdata->evt_ptr = evt_ptr;
  SchedulerEvent evt = ccn_schedule_event(this->sched(), static_cast<int>(delay.count()), &Scheduler::ScheduledAction, evdata, 0);
  if (evt_ptr != nullptr) *evt_ptr = evt;
  return evt;
}

void Scheduler::Cancel(SchedulerEvent evt) {
  if (evt == nullptr) return;
  ccn_schedule_cancel(this->sched(), evt);
}

std::chrono::microseconds Scheduler::Run(void) {
  int next = ccn_schedule_run(this->sched());
  if (next < 0) return Scheduler::kNoMore;
  return std::chrono::microseconds(next);
}

int Scheduler::ScheduledAction(ccn_schedule* sched, void* clienth, ccn_scheduled_event* ev, int flags) {
  EvData* evdata = static_cast<EvData*>(ev->evdata);
  if ((flags & CCN_SCHEDULE_CANCEL) == 0) {
    std::chrono::microseconds delay = evdata->cb();
    if (delay.count() > 0) {
      return static_cast<int>(delay.count());
    }
  }
  if (evdata->evt_ptr != nullptr) *(evdata->evt_ptr) = nullptr;
  delete evdata;
  return -1;
}

};//namespace ndnfd
