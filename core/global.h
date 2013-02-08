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

// A Global contains all global structures of a router.
// A pointer to the Global object is provided in every Element.
class Global {
 public:
  //TODO generate getter and setter for all fields

 private:
  ccnd_handle* h_;
  Ptr<PollMgr> pollmgr_;
  Ptr<Scheduler> scheduler_;
  Ptr<FaceMgr> facemgr_;
  DISALLOW_COPY_AND_ASSIGN(Global);
};

};//namespace ndnfd
#endif//NDNFD_CORE_GLOBAL_H
