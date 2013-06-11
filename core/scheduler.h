#ifndef NDNFD_CORE_SCHEDULER_H_
#define NDNFD_CORE_SCHEDULER_H_
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "core/element.h"
namespace ndnfd {

// A SchedulerEvent represents a callback to be executed later.
typedef ccn_scheduled_event* SchedulerEvent;

// Scheduler provides a mechanism to execute a callback later.
class Scheduler : public Element {
 public:
  // Callback is an event to be scheduled.
  // It should return the next time the event would be rescheduled,
  // or kNoMore to indicate the event shouldn't be rescheduled.
  typedef std::function<std::chrono::microseconds(void)> Callback;
  
  // event shouldn't be scheduled again; scheduler queue is empty
  static constexpr std::chrono::microseconds kNoMore = std::chrono::microseconds(-1);
  // event shouldn't be scheduled again, and Cancel has been called in Callback
  static constexpr std::chrono::microseconds kNoMore_NoCleanup = std::chrono::microseconds(-2);
 
  Scheduler(void);// use global schedule
  Scheduler(ccn_schedule* sched);// use local schedule; will be destroy in dtor
  virtual ~Scheduler(void);
  
  // Schedule schedules an event.
  // cb must remain valid until event happens or is cancelled.
  // If evt_ptr is not null, SchedulerEvent will be written to *evt_ptr,
  // and *evt_ptr will be cleared to nullptr when the event is no longer scheduled.
  // If cancel_old is true, the event at *evt_ptr is first cancelled.
  SchedulerEvent Schedule(std::chrono::microseconds delay, Callback cb, SchedulerEvent* evt_ptr = nullptr, bool cancel_old = false);
  
  // Cancel cancels an event. evt becomes invalid after this call.
  void Cancel(SchedulerEvent evt);
  
  // Run executes scheduled events, and returns time until next event,
  // or kNoMore to indicate the event queue is empty.
  std::chrono::microseconds Run(void);

 private:
  struct EvData {
    Scheduler* scheduler;
    Callback cb;
    SchedulerEvent* evt_ptr;
  };
  ccn_schedule* sched_;
  ccn_schedule* sched() { return this->sched_ == nullptr ? CCNDH->sched : this->sched_; };

  static int ScheduledAction(ccn_schedule* sched, void* clienth, ccn_scheduled_event* ev, int flags);

  DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

};//namespace ndnfd
#endif//NDNFD_CORE_SCHEDULER_H
