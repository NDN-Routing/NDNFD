#ifndef NDNFD_CORE_GLOBAL_H_
#define NDNFD_CORE_GLOBAL_H_
#include "util/defs.h"
#include "util/logging.h"
extern "C" {
  struct ccnd_handle;
}
namespace ndnfd {

// Member types are forward declared in global.h.
// Therefore, Global class saves T* instead of Ptr<T>.
// set_*() methods Ref() new elements and Unref() old elements.
class Logging;
class PollMgr;
class Scheduler;
class FaceMgr;
class InternalClientHandler;

// A Global contains all global structures of a router.
// A pointer to the Global object is provided in every Element.
class Global {
 public:
  Global(void);
  void Init(void);//create all objects
  ~Global(void);

  Logging* logging(void) { return &this->logging_; }
  ccnd_handle* ccndh(void) const { return this->ccndh_; }
  void set_ccndh(ccnd_handle* value);
  PollMgr* pollmgr(void) const { return this->pollmgr_; }
  void set_pollmgr(Ptr<PollMgr> value);
  Scheduler* scheduler(void) const { return this->scheduler_; }
  void set_scheduler(Ptr<Scheduler> value);
  FaceMgr* facemgr(void) const { return this->facemgr_; }
  void set_facemgr(Ptr<FaceMgr> value);
  InternalClientHandler* internal_client_handler(void) const { return this->internal_client_handler_; }
  void set_internal_client_handler(Ptr<InternalClientHandler> value);
  
 private:
  Logging logging_;
  ccnd_handle* ccndh_;
  PollMgr* pollmgr_;
  Scheduler* scheduler_;
  FaceMgr* facemgr_;
  InternalClientHandler* internal_client_handler_;
  DISALLOW_COPY_AND_ASSIGN(Global);
};

};//namespace ndnfd
#endif//NDNFD_CORE_GLOBAL_H
