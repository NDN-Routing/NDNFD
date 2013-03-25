#include "ndnfdsim.h"
#include "facemgr.h"
#include "core/scheduler.h"
namespace ndnfd {

int CcndLogger(void* loggerdata, const char* format, va_list ap) {
  char buf[512];
  int res = vsnprintf(buf, sizeof(buf), format, ap);
  
  Logging* logging = static_cast<Logging*>(loggerdata);
  logging->Log(kLLInfo, kLCCcndCore, "%s", buf);
  return res;
}

void NdnfdSim::Init(void) {
  ccnd_handle* h = ccnd_create("ndnfdsim", &CcndLogger, this->global()->logging());
  h->logpid = static_cast<int>(this->nodeid_);
  this->global()->set_ccndh(h);
  this->global()->set_facemgr(this->New<FaceMgrSim>());
  
  this->internal_client_ = this->New<InternalClientFace>();
  // TODO subclass FaceMgr, start listeners on CsmaNetDevices
  //this->global()->facemgr()->StartDefaultListeners();
}

SimClock::duration NdnfdSim::RunOnce(void) {
  // TODO mock current time
  this->internal_client_->Grab();
  std::chrono::microseconds next_scheduler_evt = this->global()->scheduler()->Run();
  this->internal_client_->Grab();
  // don't use PollMgr: sending never blocks, receiving is invoked in upcall
  return std::chrono::duration_cast<SimClock::duration>(next_scheduler_evt);
}


};//namespace ndnfd
