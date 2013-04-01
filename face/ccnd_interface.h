#ifndef NDNFD_FACE_CCND_INTERFACE_H_
#define NDNFD_FACE_CCND_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "ccnd/ccnd_private.h"

struct face* face_from_faceid(struct ccnd_handle *h, unsigned faceid);
void ccnd_send(struct ccnd_handle* h, struct face* face, const void* data, size_t size);

#ifdef __cplusplus
}

#include "face.h"
#include "facemgr.h"

namespace ndnfd {

// CcndFaceInterface moves messages between ccnd core and NDNFD face abstration.
class CcndFaceInterface : public Element {
 public:
  CcndFaceInterface(void) {}
  virtual ~CcndFaceInterface(void) {}
  
  // BindFace is called by FaceMgr.AddFace.
  // It binds Receive to face.Receive port.
  void BindFace(Ptr<Face> face);

  // Send sends a CCNB message through a face.
  void Send(FaceId faceid, uint8_t* msg, size_t length);
  
  // last message passed to process_input_message,
  // in case strategy wants to inspect (eg. to get incoming_sender)
  Ptr<Message> last_received_message_;
  
 private:
  // Receive is bound to face.Receive port.
  // When a message is received, it is handed over to ccnd core
  // through process_input_message.
  void Receive(Ptr<Message> message);

  DISALLOW_COPY_AND_ASSIGN(CcndFaceInterface);
};

};//namespace ndnfd
#endif
#endif//NDNFD_FACE_CCND_INTERFACE_H
