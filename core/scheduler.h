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
  
  static constexpr std::chrono::microseconds kNoMore = std::chrono::microseconds(-1);
 
  Scheduler(void) {}
  virtual ~Scheduler(void) {}
  
  // Schedule schedules an event.
  // cb must remain valid until event happens or is cancelled.
  SchedulerEvent Schedule(std::chrono::microseconds delay, Callback cb);
  
  // Cancel cancels an event. evt becomes invalid after this call.
  void Cancel(SchedulerEvent evt);
  
  // Run executes scheduled events, and returns time until next event,
  // or kNoMore to indicate the event queue is empty.
  std::chrono::microseconds Run(void);

 private:
  struct EvData {
    Scheduler* scheduler;
    Callback cb;
  };

  ccn_schedule* sched() { return this->global()->ccndh()->sched; };
  static int ScheduledAction(ccn_schedule* sched, void* clienth, ccn_scheduled_event* ev, int flags);

  DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

};//namespace ndnfd
#endif//NDNFD_CORE_SCHEDULER_H
