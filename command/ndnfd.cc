#include "ndnfd.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "face/facemgr.h"
#include "core/pollmgr.h"
#include "core/scheduler.h"
#include "face/ip.h"
namespace ndnfd {

void NdnfdProgram::Init(void) {
  ccnd_handle* h = ccnd_create("ndnfd", &CcndLogger, this->global()->logging());
  this->global()->set_ccndh(h);
  
  this->internal_client_ = this->New<InternalClientFace>();
  this->global()->facemgr()->StartDefaultListeners();
  
  bool ok; NetworkAddress addr;

  std::tie(ok, addr) = IpAddressVerifier::Parse("131.179.196.46:9695");//b.hub.ndn.ucla.edu
  assert(ok);
  Ptr<Face> tcp_Bhub = this->global()->facemgr()->tcp_factory()->Connect(addr);
  this->ccndc_add(tcp_Bhub->id(), "/");

  //std::tie(ok, addr) = IpAddressVerifier::Parse("192.168.3.2:9695");
  //assert(ok);
  //Ptr<Face> udp_2 = this->udp_channel_->GetFace(addr);
  //this->ccndc_add(udp_2->id(), "/example");
}

void NdnfdProgram::Run(void) {
  while (true) {
    this->internal_client_->Grab();
    std::chrono::microseconds next_scheduler_evt = this->global()->scheduler()->Run();
    this->internal_client_->Grab();
    std::chrono::milliseconds poll_timeout = next_scheduler_evt.count()<0 ? PollMgr::kNoTimeout : std::chrono::duration_cast<std::chrono::milliseconds>(next_scheduler_evt);
    this->global()->pollmgr()->Poll(poll_timeout);
  }
}

void NdnfdProgram::ccndc_add(FaceId faceid, std::string prefix) {
  char buf[256];
  snprintf(buf, sizeof(buf), "bash -c 'sleep 1 && ccndc add %s face %"PRI_FaceId"' &", prefix.c_str(), faceid);
  int res = system(buf);
  if (res == 0) {}
}

int CcndLogger(void* loggerdata, const char* format, va_list ap) {
  char buf[512];
  int res = vsnprintf(buf, sizeof(buf), format, ap);
  
  Logging* logging = static_cast<Logging*>(loggerdata);
  logging->Log(kLLInfo, kLCCcndCore, "%s", buf);
  return res;
}

};//namespace ndnfd

int main(int argc, char** argv) {
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


