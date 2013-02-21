#include "ccnd_interface.h"
#include "face/facemgr.h"
#include "message/ccnb.h"
extern "C" {
void process_input_message(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, int pdu_ok);
}
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

void CcndFaceInterface::BindFace(Ptr<Face> face) {
  face->Receive = std::bind(&CcndFaceInterface::Receive, this, std::placeholders::_1);
}

void CcndFaceInterface::Receive(Ptr<Message> message) {
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(message->incoming_face());
  if (in_face == nullptr) {
    this->Log(kLLError, kLCFace, "CcndFaceInterface::Receive face %"PRI_FaceId" does not exist", message->incoming_face());
    return;
  }
  if (in_face->kind() == FaceKind::kMulticast && !in_face->CanSend()) {
    // this is a fallback face
    // TODO create an unicast face for message->incoming_send(),
    //      and set message->incoming_face() to the new face
    assert(false);//not implemented
  }
  
  CcnbMessage* msg = static_cast<CcnbMessage*>(PeekPointer(message));
  process_input_message(this->global()->ccndh(), in_face->ccnd_face(), static_cast<unsigned char*>(msg->msg()), msg->length(), 0);
}

void CcndFaceInterface::Send(FaceId faceid, uint8_t* msg, size_t length) {
  Ptr<Face> out_face = this->global()->facemgr()->GetFace(faceid);
  if (out_face == nullptr) {
    this->Log(kLLError, kLCFace, "CcndFaceInterface::Send face %"PRI_FaceId" does not exist", faceid);
    return;
  }
  if (!out_face->CanSend()) {
    this->Log(kLLError, kLCFace, "CcndFaceInterface::Send Face(%"PRI_FaceId")::CanSend is false", out_face->id());
    return;
  }
  
  Ptr<CcnbMessage> message = new CcnbMessage(msg, length);
  out_face->Send(message);
}

};//namespace ndnfd

