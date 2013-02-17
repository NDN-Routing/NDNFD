#include "core/global.h"
#include "core/pollmgr.h"
#include "core/scheduler.h"
#include "face/facemgr.h"
namespace ndnfd {

Global::Global(void) {
  this->ccndh_ = nullptr;
  this->pollmgr_ = nullptr;
  this->scheduler_ = nullptr;
  this->facemgr_ = nullptr;
}

Global::~Global(void) {
  this->set_pollmgr(nullptr);
  this->set_scheduler(nullptr);
  this->set_facemgr(nullptr);
}

void Global::set_pollmgr(Ptr<PollMgr> value) {
  if (this->pollmgr_ != nullptr) {
    this->pollmgr_->Unref();
  }
  this->pollmgr_ = GetPointer(value);
}

void Global::set_scheduler(Ptr<Scheduler> value) {
  if (this->scheduler_ != nullptr) {
    this->scheduler_->Unref();
  }
  this->scheduler_ = GetPointer(value);
}

void Global::set_facemgr(Ptr<FaceMgr> value) {
  if (this->facemgr_ != nullptr) {
    this->facemgr_->Unref();
  }
  this->facemgr_ = GetPointer(value);
}

};//namespace ndnfd
