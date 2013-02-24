#include "ndnfd.h"
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "face/facemgr.h"
#include "core/pollmgr.h"
#include "core/scheduler.h"
#include "message/ccnb.h"
namespace ndnfd {

void NdnfdProgram::Init(void) {
  ccnd_handle* h = ccnd_create("ndnfd", &CcndLogger, this->global()->logging());
  this->global()->set_ccndh(h);
  
  this->internal_client_ = this->New<InternalClientFace>();
  this->unix_face_factory_ = this->New<UnixFaceFactory>(this->New<CcnbWireProtocol>(true));
  this->unix_listener_ = this->unix_face_factory_->Listen("/tmp/.ccnd.sock");
}

void NdnfdProgram::Run(void) {
  while (true) {
    this->internal_client_->Grab();
    this->global()->scheduler()->Run();
    this->internal_client_->Grab();
    this->global()->pollmgr()->Poll(std::chrono::milliseconds(1));
  }
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


