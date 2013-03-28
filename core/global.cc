#include "core/global.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "core/pollmgr.h"
#include "core/scheduler.h"
#include "face/facemgr.h"
#include "core/internal_client_handler.h"
#include "core/nameprefix_table.h"
namespace ndnfd {

Global::Global(void) {
  this->ccndh_ = nullptr;
  this->pollmgr_ = nullptr;
  this->scheduler_ = nullptr;
  this->facemgr_ = nullptr;
  this->internal_client_handler_ = nullptr;
  this->npt_ = nullptr;
}

void Global::Init(void) {
  Ptr<Element> first = Element::MakeFirstElement(this);
  this->set_ccndh(new ccnd_handle());
  this->set_pollmgr(first->New<PollMgr>());
  this->set_scheduler(first->New<Scheduler>());
  this->set_facemgr(first->New<FaceMgr>());
  this->set_internal_client_handler(first->New<InternalClientHandler>());
  this->set_npt(first->New<NamePrefixTable>());
}

Global::~Global(void) {
  this->set_ccndh(nullptr);
  this->set_pollmgr(nullptr);
  this->set_scheduler(nullptr);
  this->set_facemgr(nullptr);
  this->set_internal_client_handler(nullptr);
  this->set_npt(nullptr);
}

void Global::set_ccndh(ccnd_handle* value) {
  if (this->ccndh_ != nullptr) {
    this->ccndh_->ndnfd_global = nullptr;
    delete this->ccndh_;
  }
  if (value != nullptr) value->ndnfd_global = this;
  this->ccndh_ = value;
}

#define GLOBAL_DEF_SETTER(field,type) \
void Global::set_##field(Ptr<type> value) { \
  if (this->field##_ != nullptr) { \
    this->field##_->Unref(); \
  } \
  this->field##_ = GetPointer(value); \
}
GLOBAL_DEF_SETTER(pollmgr,PollMgr);
GLOBAL_DEF_SETTER(scheduler,Scheduler);
GLOBAL_DEF_SETTER(facemgr,FaceMgr);
GLOBAL_DEF_SETTER(internal_client_handler,InternalClientHandler);
GLOBAL_DEF_SETTER(npt,NamePrefixTable);

};//namespace ndnfd
