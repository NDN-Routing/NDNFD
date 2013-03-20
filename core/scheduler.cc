#include "scheduler.h"
extern "C" {
void ccnd_gettime(const struct ccn_gettime *self, struct ccn_timeval *result);
}
namespace ndnfd {

constexpr std::chrono::microseconds Scheduler::kNoMore;

SchedulerEvent Scheduler::Schedule(std::chrono::microseconds delay, Callback cb) {
  assert(cb != nullptr);
  if (this->sched() == nullptr) {
    this->Log(kLLError, kLCScheduler, "Scheduler::Schedule sched is null");
    return nullptr;
  }
  EvData* evdata = new EvData();
  evdata->scheduler = this;
  evdata->cb = cb;
  return ccn_schedule_event(this->sched(), static_cast<int>(delay.count()), &Scheduler::ScheduledAction, evdata, 0);
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
  delete evdata;
  return -1;
}

};//namespace ndnfd
