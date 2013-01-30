#ifndef NDNFD_CORE_SCHEDULER_H_
#define NDNFD_CORE_SCHEDULER_H_
#include "util/defs.h"
#include <poll.h>
namespace ndnfd {

class Scheduler : Object {
  public:
    void Run(void);

  private:
    DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

};//namespace ndnfd
#endif//NDNFD_CORE_SCHEDULER_H
