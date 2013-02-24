#ifndef NDNFD_CORE_SCHEDULER_H_
#define NDNFD_CORE_SCHEDULER_H_
#include "core/element.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
namespace ndnfd {

// A Scheduler provides a mechanism to periodically execute a calllback.
class Scheduler : public Element {
 public:
  Scheduler(void) {}
  virtual ~Scheduler(void) {}
  
  void Run(void) { ccn_schedule_run(this->global()->ccndh()->sched); }

 private:
  DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

};//namespace ndnfd
#endif//NDNFD_CORE_SCHEDULER_H
