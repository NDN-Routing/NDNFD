#include "internal_client.h"
#include "face/facemgr.h"
extern "C" {
void process_internal_client_buffer(struct ccnd_handle* h);
}
namespace ndnfd {

void InternalClientFace::Init(void) {
  this->set_kind(FaceKind::kInternal);
  this->set_face_thread(this->global()->facemgr()->integrated_face_thread());
  this->global()->facemgr()->AddFace(this);
  CCNDH->face0 = this->ccnd_face();
  int res = ccnd_internal_client_start(CCNDH);
  if (res < 0) {
    this->Log(kLLError, kLCCcndFace, "InternalClientFace::Init ccnd_internal_client_start fails");
    assert(false);
  }
}

InternalClientFace::~InternalClientFace(void) {
  ccnd_internal_client_stop(CCNDH);
  CCNDH->face0 = nullptr;
  this->global()->facemgr()->RemoveFace(this);
}

FaceDescription InternalClientFace::GetDescription(void) const {
  FaceDescription d;
  d.proto_ = "INTERNAL";
  d.peer_ = "INTERNAL_CLIENT";
  return d;
}

void InternalClientFace::Send(Ptr<const Message> message) {
  this->Log(kLLDebug, kLCCcndFace, "InternalClientFace::Send interest_faceid=%u", CCNDH->interest_faceid);
  const CcnbMessage* msg = static_cast<const CcnbMessage*>(PeekPointer(message));
  ccn_dispatch_message(this->internal_client(), const_cast<uint8_t*>(msg->msg()), msg->length());
}

void InternalClientFace::Grab(void) {
  process_internal_client_buffer(CCNDH);
}


};//namespace ndnfd
