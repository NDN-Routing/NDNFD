#ifndef NDNFD_CORE_SCHEDULER_H_
#define NDNFD_CORE_SCHEDULER_H_
#include "core/element.h"
namespace ndnfd {

// A Scheduler provides a mechanism to periodically execute a calllback.
class Scheduler : public Element {
 public:
  Scheduler(void);
  virtual ~Scheduler(void) {}
  
  void Run(void);

 private:
  //TODO design private fields (as a call to ccn scheduler)
  DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

};//namespace ndnfd
#endif//NDNFD_CORE_SCHEDULER_H
