#include "ccnd_interface.h"
#include "face/facemgr.h"
#include "message/ccnb.h"
extern "C" {
void register_new_face(struct ccnd_handle *h, struct face *face);
void process_input_message(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, int pdu_ok);
}
using ndnfd::Global;
using ndnfd::Face;
using ndnfd::FaceId;
using ndnfd::FaceMgr;
using ndnfd::kLCFace;

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

int ccnd_req_newface(struct ccnd_handle *h, const unsigned char *msg, size_t size, struct ccn_charbuf *reply_body) {
  using ndnfd::kLLError;
  Global* global = ccnd_ndnfdGlobal(h);
  global->logging()->Log(kLLError, kLCFace, "ccnd_req_newface not implemented");
  //TODO decode msg as face_instance, verify trusted, call FaceMgr::FaceMgmtRequest, append response to reply_body
  return -1;
}

int ccnd_req_destroyface(struct ccnd_handle *h, const unsigned char *msg, size_t size, struct ccn_charbuf *reply_body) {
  using ndnfd::kLLError;
  Global* global = ccnd_ndnfdGlobal(h);
  global->logging()->Log(kLLError, kLCFace, "ccnd_req_destroyface not implemented");
  //TODO decode msg as face_instance, verify trusted, call FaceMgr::FaceMgmtRequest, append response to reply_body
  return -1;
}

namespace ndnfd {

void CcndFaceInterface::BindFace(Ptr<Face> face) {
  face->Receive = std::bind(&CcndFaceInterface::Receive, this, std::placeholders::_1);
  register_new_face(this->global()->ccndh(), face->ccnd_face());
}

void CcndFaceInterface::Receive(Ptr<Message> message) {
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(message->incoming_face());
  if (in_face == nullptr) {
    this->Log(kLLError, kLCFace, "CcndFaceInterface::Receive face %"PRI_FaceId" does not exist", message->incoming_face());
    return;
  }
  if (in_face->kind() == FaceKind::kMulticast && !in_face->CanSend()) {
    Ptr<Face> uface = this->global()->facemgr()->MakeUnicastFace(in_face, message->incoming_sender());
    message->set_incoming_face(uface->id());
    this->Log(kLLInfo, kLCFace, "CcndFaceInterface::Receive fallback face %"PRI_FaceId", creating unicast face %"PRI_FaceId"", in_face->id(), uface->id());
  }
  
  CcnbMessage* msg = static_cast<CcnbMessage*>(PeekPointer(message));
  assert(msg->Verify());
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

