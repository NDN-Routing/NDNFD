#include "ndnfdsim.h"
#include <cstdarg>
#include "core/scheduler.h"
#include "global.h"
#include "facemgr.h"
#include "strategy/strategy.h"
extern "C" {
#include "ndnld/ndnld.h"
uint32_t WTHZ_value(void);
}
namespace ndnfd {

void NdnfdSim::Init(void) {
  ccnd_handle* h = ccnd_create("ndnfdsim", &Logging::CcndLogger, this->global()->logging());
  h->logpid = static_cast<int>(THIS_SIMGLOBAL->nodeid());
  h->ticktock.gettime = &NdnfdSim::CcndGetTime;
  struct ccn_timeval dummy;
  h->ticktock.gettime(&h->ticktock, &dummy);
  h->starttime = h->sec;
  h->starttime_usec = h->usec;
  this->global()->set_ccndh(h);
  this->global()->strategy()->Init2();
  
  this->internal_client_ = this->New<InternalClientFace>();
  this->global()->facemgr()->StartDefaultListeners();
}

NdnfdSim::~NdnfdSim(void) {
}

void NdnfdSim::CcndGetTime(const ccn_gettime* self, ccn_timeval* result) {
  ccnd_handle* h = const_cast<ccnd_handle*>(static_cast<const ccnd_handle*>(self->data));

  int64_t now_us = ns3::Now().GetMicroSeconds();
  result->s = static_cast<long>(now_us / 1000000);
  result->micros = static_cast<unsigned>(now_us % 1000000);
  
  long sdelta = result->s - h->sec; int udelta = result->micros + h->sliver - h->usec;
  while (udelta < 0) { udelta += 1000000; sdelta -= 1; }
  h->sec = result->s; h->usec = result->micros;

  // ns3 time won't run backwards or take huge steps
  ccn_wrappedtime delta = static_cast<unsigned>(udelta) / (1000000U / WTHZ_value());
  h->sliver = udelta - delta * (1000000U / WTHZ_value());
  delta += static_cast<unsigned>(sdelta) * WTHZ_value();
  h->wtnow += delta;

  // also update time in ndnld
  DateTime_mockNow(static_cast<DateTime>(now_us / 1000));
}

void NdnfdSim::Start(void) {
  ns3::Simulator::ScheduleWithContext(THIS_SIMGLOBAL->nodeid(), ns3::MicroSeconds(0), &NdnfdSim::RunOnce, this);
}

void NdnfdSim::ScheduleOnNextRun(std::function<void()> action) {
  this->next_run_actions_.push(action);
  this->run_evt_.Cancel();
  this->run_evt_ = ns3::Simulator::ScheduleNow(&NdnfdSim::RunOnce, this);
}

void NdnfdSim::RunOnce(void) {
  //this->Log(kLLDebug, kLCSim, "NdnfdSim(%" PRIu32 ")::RunOnce() at %0.6f", THIS_SIMGLOBAL->nodeid(), ns3::Now().GetSeconds());

  ccn_timeval dummy;
  this->global()->ccndh()->ticktock.gettime(&(this->global()->ccndh()->ticktock), &dummy);
  
  while (!this->next_run_actions_.empty()) {
    this->next_run_actions_.front()();
    this->next_run_actions_.pop();
  }

  this->internal_client_->Run();
  std::chrono::microseconds next_scheduler_evt = this->global()->scheduler()->Run();

  uint64_t next_us = 10000;
  if (next_scheduler_evt != Scheduler::kNoMore) {
    next_us = std::min(next_us, static_cast<uint64_t>(next_scheduler_evt.count()));
  }
  this->run_evt_ = ns3::Simulator::Schedule(ns3::MicroSeconds(next_us), &NdnfdSim::RunOnce, this);
}

};//namespace ndnfd
