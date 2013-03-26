#include "ndnfdsim.h"
#include <ns3/simulator.h>
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
  h->ticktock.gettime = &NdnfdSim::CcndGetTime;
  struct ccn_timeval dummy;
  h->ticktock.gettime(&h->ticktock, &dummy);
  h->starttime = h->sec;
  h->starttime_usec = h->usec;
  this->global()->set_ccndh(h);
  
  this->internal_client_ = this->New<InternalClientFace>();
  this->global()->facemgr()->StartDefaultListeners();
}

void NdnfdSim::CcndGetTime(const ccn_gettime* self, ccn_timeval* result) {
  ccnd_handle* h = const_cast<ccnd_handle*>(static_cast<const ccnd_handle*>(self->data));
  const uint16_t WTHZ = 500U;

  int64_t now_us = ns3::Now().GetMicroSeconds();
  result->s = static_cast<long>(now_us / 1000000);
  result->micros = static_cast<unsigned>(now_us % 1000000);

  long sdelta = result->s - h->sec; int udelta = result->micros + h->sliver - h->usec;
  while (udelta < 0) { udelta += 1000000; sdelta -= 1; }
  h->sec = result->s; h->usec = result->micros;

  // ns3 time won't run backwards or take huge steps
  ccn_wrappedtime delta = static_cast<unsigned>(udelta) / (1000000U / WTHZ);
  h->sliver = udelta - delta * (1000000U / WTHZ);
  delta += static_cast<unsigned>(sdelta) * WTHZ;
  h->wtnow += delta;
}

void NdnfdSim::Start(void) {
  ns3::Simulator::ScheduleNow(&NdnfdSim::RunOnce, this);
}

void NdnfdSim::RunOnce(void) {
  //this->Log(kLLDebug, kLCSim, "NdnfdSim(%" PRIu32 ")::RunOnce() at %0.6f", this->nodeid_, ns3::Now().GetSeconds());

  this->internal_client_->Grab();
  std::chrono::microseconds next_scheduler_evt = this->global()->scheduler()->Run();
  this->internal_client_->Grab();
  // don't use PollMgr: sending never blocks, receiving is invoked in upcall

  uint64_t next_us = 5000;
  if (next_scheduler_evt != Scheduler::kNoMore) {
    next_us = std::min(next_us, static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(next_scheduler_evt).count()));
  }
  ns3::Simulator::Schedule(ns3::MicroSeconds(next_us), &NdnfdSim::RunOnce, this);
}

};//namespace ndnfd
