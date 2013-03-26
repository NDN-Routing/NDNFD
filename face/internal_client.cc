#include "internal_client.h"
#include "face/facemgr.h"
extern "C" {
void process_internal_client_buffer(struct ccnd_handle* h);
}
namespace ndnfd {

void InternalClientFace::Init(void) {
  this->set_kind(FaceKind::kInternal);
  this->global()->facemgr()->AddFace(this);
  this->global()->ccndh()->face0 = this->ccnd_face();
  int res = ccnd_internal_client_start(this->global()->ccndh());
  if (res < 0) {
    this->Log(kLLError, kLCFace, "InternalClientFace::Init ccnd_internal_client_start fails");
    assert(false);
  }
}

InternalClientFace::~InternalClientFace(void) {
  ccnd_internal_client_stop(this->global()->ccndh());
  this->global()->ccndh()->face0 = nullptr;
  this->global()->facemgr()->RemoveFace(this);
}

void InternalClientFace::Send(Ptr<Message> message) {
  CcnbMessage* msg = static_cast<CcnbMessage*>(PeekPointer(message));
  ccn_dispatch_message(this->internal_client(), const_cast<uint8_t*>(msg->msg()), msg->length());
}

void InternalClientFace::Grab(void) {
  process_internal_client_buffer(this->global()->ccndh());
}


};//namespace ndnfd
