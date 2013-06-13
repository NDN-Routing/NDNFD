#include "ccnd_interface.h"
#include "face/facemgr.h"
#include "message/interest.h"
#include "message/contentobject.h"
#include "strategy/layer.h"
extern "C" {
void register_new_face(struct ccnd_handle *h, struct face *face);
void process_input_message(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, int pdu_ok);
}
using ndnfd::Ptr;
using ndnfd::Global;
using ndnfd::Face;
using ndnfd::FaceId;
using ndnfd::FaceMgr;

struct face* face_from_faceid(struct ccnd_handle *h, unsigned faceid) {
  Global* global = ccnd_ndnfdGlobal(h);
  assert(global->IsCoreThread());
  Ptr<Face> face = global->facemgr()->GetFace(static_cast<FaceId>(faceid));
  if (face == nullptr) return nullptr;
  return face->ccnd_face();
}

void stuff_and_send(struct ccnd_handle* h, struct face* face, const unsigned char *data1, size_t size1, const unsigned char *data2, size_t size2, const char *tag, int lineno) {
  Global* global = ccnd_ndnfdGlobal(h);
  global->facemgr()->ccnd_face_interface()->Send(static_cast<FaceId>(face->faceid), data1, size1, data2, size2);
}

namespace ndnfd {

void CcndFaceInterface::BindFaceThread(Ptr<FaceThread> face_thread) {
  face_thread->Receive = std::bind(&CcndFaceInterface::Receive, this, std::placeholders::_1);
}

void CcndFaceInterface::BindFace(Ptr<Face> face) {
  //face->Receive = std::bind(&CcndFaceInterface::Receive, this, std::placeholders::_1);
  register_new_face(CCNDH, face->ccnd_face());
}

void CcndFaceInterface::Receive(Ptr<Message> message) {
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(message->incoming_face());
  if (in_face == nullptr) {
    this->Log(kLLError, kLCCcndFace, "CcndFaceInterface::Receive face %" PRI_FaceId " does not exist", message->incoming_face());
    return;
  }
  if (in_face->kind() == FaceKind::kMulticast && !in_face->CanSend()) {
    Ptr<Face> uface = this->global()->facemgr()->MakeUnicastFace(in_face, message->incoming_sender());
    this->Log(kLLInfo, kLCCcndFace, "CcndFaceInterface::Receive fallback face %" PRI_FaceId ", creating unicast face %" PRI_FaceId "", in_face->id(), uface->id());
    in_face = uface;
    message->set_incoming_face(uface->id());
  }
  
  this->last_received_message_ = message;
  
  if (message->type() == InterestMessage::kType) {
    InterestMessage* interest = static_cast<InterestMessage*>(PeekPointer(message));
    this->global()->sl()->OnInterest(interest);
  } else if (message->type() == ContentObjectMessage::kType) {
    ContentObjectMessage* co = static_cast<ContentObjectMessage*>(PeekPointer(message));
    this->global()->sl()->OnContent(co);
  } else if (message->type() == NackMessage::kType) {
    NackMessage* nack = static_cast<NackMessage*>(PeekPointer(message));
    this->global()->sl()->OnNack(nack);
  } else {
    assert(false);
  }
}

void CcndFaceInterface::Send(FaceId faceid, const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2) {
  Ptr<Face> out_face = this->global()->facemgr()->GetFace(faceid);
  if (out_face == nullptr) {
    this->Log(kLLError, kLCCcndFace, "CcndFaceInterface::Send face %" PRI_FaceId " does not exist", faceid);
    return;
  }
  if (!out_face->CanSend()) {
    this->Log(kLLError, kLCCcndFace, "CcndFaceInterface::Send Face(%" PRI_FaceId ")::CanSend is false", out_face->id());
    return;
  }
  
  Ptr<Buffer> buf = new Buffer(size1, 0, size2);
  memcpy(buf->mutable_data(), data1, size1);
  if (size2 > 0) memcpy(buf->Put(size2), data2, size2);
  Ptr<CcnbMessage> message = new CcnbMessage(buf->data(), buf->length());
  message->set_source_buffer(buf);
  out_face->face_thread()->Send(out_face->id(), message);
}

};//namespace ndnfd

