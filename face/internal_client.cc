#include "internal_client.h"
#include "face/facemgr.h"

void ccnd_internal_client_has_somthing_to_say(struct ccnd_handle* h) {
  // This is used for sending adjancency Interest, which is not supported.
}

namespace ndnfd {

void InternalClientFace::Init(void) {
  this->set_kind(FaceKind::kInternal);
  this->set_face_thread(this->global()->facemgr()->integrated_face_thread());
  this->global()->facemgr()->AddFace(this);
  CCNDH->face0 = this->native();

  this->wp_ = this->New<CcnbWireProtocol>(true);
  assert(this->wp_->IsStateful());
  this->wps_ = this->wp_->CreateState(NetworkAddress());

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
  bool ok; std::list<Ptr<Buffer>> pkts;
  std::tie(ok, pkts) = this->wp_->Encode(NetworkAddress(), this->wps_, message);
  if (!ok) {
    this->set_status(FaceStatus::kProtocolError);
    return;
  }
  
  for (Ptr<Buffer> pkt : pkts) {
    this->CountBytesOut(pkt->length());
    ccn_dispatch_message(this->internal_client(), const_cast<uint8_t*>(pkt->data()), pkt->length());
  }
}

void InternalClientFace::Run(void) {
  ccn_charbuf* inbuf = ccn_grab_buffered_output(CCNDH->internal_client);
  if (inbuf == nullptr) return;
  this->CountBytesIn(inbuf->length);
  
  Ptr<Buffer> pkt = this->wps_->GetReceiveBuffer();
  if (pkt->length() == 0) {
    pkt->Swap(inbuf);
  } else {
    memcpy(pkt->Put(inbuf->length), inbuf->buf, inbuf->length);
  }
  ccn_charbuf_destroy(&inbuf);
  
  bool ok; std::list<Ptr<Message>> msgs;
  std::tie(ok, msgs) = this->wp_->Decode(NetworkAddress(), this->wps_, pkt);
  if (!ok) {
    this->set_status(FaceStatus::kProtocolError);
    return;
  }

  for (Ptr<Message> msg : msgs) {
    this->ReceiveMessage(msg);
  }
}


};//namespace ndnfd
