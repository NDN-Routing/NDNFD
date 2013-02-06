#ifndef NDNFD_CORE_SCHEDULER_H_
#define NDNFD_CORE_SCHEDULER_H_
#include "core/element.h"
namespace ndnfd {

class Scheduler : public Element {
  public:
    void Run(void);

  private:
    DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

};//namespace ndnfd
#endif//NDNFD_CORE_SCHEDULER_H
