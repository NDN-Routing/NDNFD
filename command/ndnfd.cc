#include "ndnfd.h"
#include <cstdio>
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "face/facemgr.h"
#include "core/pollmgr.h"
#include "core/scheduler.h"
#include "face/ip.h"
#include "face/ether.h"
#include "strategy/strategy.h"
namespace ndnfd {

void NdnfdProgram::Init(void) {
  ccnd_handle* h = ccnd_create("ndnfd", &Logging::CcndLogger, this->global()->logging());
  this->global()->set_ccndh(h);
  this->global()->strategy()->Init2();
  this->global()->facemgr()->AddFaceThreads(2);
  
  this->internal_client_ = this->New<InternalClientFace>();
  this->global()->facemgr()->StartDefaultListeners();
  
  //for (DgramFace* udp_mcast_face : this->global()->facemgr()->udp_mcast_faces()) {
  //  this->ccndc_add(udp_mcast_face->id(), "/");
  //}

  //for (auto ether_tuple : this->global()->facemgr()->ether_channels()) {
  //  Ptr<DgramFace> ether_mcast_face = std::get<2>(ether_tuple);
  //  this->ccndc_add(ether_mcast_face->id(), "/");
  //}
}

void NdnfdProgram::Run(void) {
  while (true) {
    this->internal_client_->Run();
    this->global()->scheduler()->Run();
    this->internal_client_->Run();
    std::chrono::microseconds next_scheduler_evt = this->global()->scheduler()->Run();
    std::chrono::milliseconds poll_timeout = next_scheduler_evt.count()<0 ? PollMgr::kNoTimeout : std::chrono::duration_cast<std::chrono::milliseconds>(next_scheduler_evt);
    this->global()->pollmgr()->Poll(poll_timeout);
  }
}

void NdnfdProgram::ccndc_add(FaceId faceid, std::string prefix) {
  char buf[256];
  snprintf(buf, sizeof(buf), "bash -c 'sleep 1 && ccndc add %s face %" PRI_FaceId "' &", prefix.c_str(), faceid);
  int res = system(buf);
  if (res == 0) {}
}

};//namespace ndnfd

int main(int argc, char** argv) {
  using ndnfd::Ptr;
  using ndnfd::Global;
  using ndnfd::Element;
  using ndnfd::NdnfdProgram;
  
  Global* global = new Global();
  global->Init();
  Ptr<NdnfdProgram> program = Element::MakeFirstElement(global)->New<NdnfdProgram>(argc, argv);
  program->Run();
  delete global;
  return 0;
}


