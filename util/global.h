#ifndef NDNFD_UTIL_GLOBAL_H_
#define NDNFD_UTIL_GLOBAL_H_
#include "util/defs.h"
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

  private:
    ccnd_handle* h_;
    PollMgr pollmgr_;
    DISALLOW_COPY_AND_ASSIGN(Global);
};

};//namespace ndnfd
#endif//NDNFD_UTIL_OBJECT_H
