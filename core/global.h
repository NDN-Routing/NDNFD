#ifndef NDNFD_CORE_GLOBAL_H_
#define NDNFD_CORE_GLOBAL_H_
#include "core/defs.h"
extern "C" {
  struct ccnd_handle;
}
namespace ndnfd {

class PollMgr;

class Global {
  public:
    ccnd_handle* h(void) const { return this->h_; }
    void set_h(ccnd_handle* value) { this->h_ = value; }
    PollMgr pollmgr(void) const { return this->pollmgr_; }
    void set_pollmgr(PollMgr value) { this->pollmgr_ = value; }
    Scheduler scheduler(void) const { return this->scheduler_; }
    void set_scheduler(Scheduler value) { this->scheduler_ = value; }

  private:
    ccnd_handle* h_;
    PollMgr pollmgr_;
    Scheduler scheduler_;
    DISALLOW_COPY_AND_ASSIGN(Global);
};

};//namespace ndnfd
#endif//NDNFD_CORE_GLOBAL_H
