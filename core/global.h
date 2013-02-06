#ifndef NDNFD_CORE_GLOBAL_H_
#define NDNFD_CORE_GLOBAL_H_
#include "util/defs.h"
extern "C" {
  struct ccnd_handle;
}
namespace ndnfd {

class PollMgr;
class Scheduler;
class FaceMgr;

class Global {
  public:
    //getter and setter for all fields

  private:
    ccnd_handle* h_;
    Ptr<PollMgr> pollmgr_;
    Ptr<Scheduler> scheduler_;
    Ptr<FaceMgr> facemgr_;
    DISALLOW_COPY_AND_ASSIGN(Global);
};

};//namespace ndnfd
#endif//NDNFD_CORE_GLOBAL_H
