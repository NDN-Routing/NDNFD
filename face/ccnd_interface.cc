#include "ccnd_interface.h"
#include "face/facemgr.h"
#include "message/interest.h"
#include "message/contentobject.h"
extern "C" {
void register_new_face(struct ccnd_handle *h, struct face *face);
void process_input_message(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, int pdu_ok);
void process_incoming_interest2(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, struct ccn_parsed_interest* pi, struct ccn_indexbuf* comps);
void process_incoming_content2(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, struct ccn_parsed_ContentObject* co);
}
using ndnfd::Ptr;
using ndnfd::Global;
using ndnfd::Face;
using ndnfd::FaceId;
using ndnfd::FaceMgr;

struct face* face_from_faceid(struct ccnd_handle *h, unsigned faceid) {
  Global* global = ccnd_ndnfdGlobal(h);
  Ptr<Face> face = global->facemgr()->GetFace(static_cast<FaceId>(faceid));
  if (face == nullptr) return nullptr;
  return face->ccnd_face();
}

void ccnd_send(struct ccnd_handle* h, struct face* face, const void* data, size_t size) {
  Global* global = ccnd_ndnfdGlobal(h);
  global->facemgr()->ccnd_face_interface()->Send(static_cast<FaceId>(face->faceid), const_cast<uint8_t*>(static_cast<const uint8_t*>(data)), size);
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
  
  switch (message->type()) {
    case InterestMessage::kType: {
      InterestMessage* interest = static_cast<InterestMessage*>(PeekPointer(message));
      process_incoming_interest2(CCNDH, in_face->ccnd_face(), static_cast<unsigned char*>(const_cast<uint8_t*>(interest->msg())), interest->length(), const_cast<ccn_parsed_interest*>(interest->parsed()), const_cast<ccn_indexbuf*>(interest->comps()));
    } break;
    case ContentObjectMessage::kType: {
      ContentObjectMessage* co = static_cast<ContentObjectMessage*>(PeekPointer(message));
      process_incoming_content2(CCNDH, in_face->ccnd_face(), static_cast<unsigned char*>(const_cast<uint8_t*>(co->msg())), co->length(), const_cast<ccn_parsed_ContentObject*>(co->parsed()));
    } break;
    default: assert(false); break;
  }
}

void CcndFaceInterface::Send(FaceId faceid, uint8_t* msg, size_t length) {
  Ptr<Face> out_face = this->global()->facemgr()->GetFace(faceid);
  if (out_face == nullptr) {
    this->Log(kLLError, kLCCcndFace, "CcndFaceInterface::Send face %" PRI_FaceId " does not exist", faceid);
    return;
  }
  if (!out_face->CanSend()) {
    this->Log(kLLError, kLCCcndFace, "CcndFaceInterface::Send Face(%" PRI_FaceId ")::CanSend is false", out_face->id());
    return;
  }
  
  Ptr<CcnbMessage> message = new CcnbMessage(msg, length);
  out_face->face_thread()->Send(out_face->id(), message);
}

};//namespace ndnfd

