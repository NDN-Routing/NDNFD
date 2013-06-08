#ifndef NDNFD_FACE_CCND_INTERFACE_H_
#define NDNFD_FACE_CCND_INTERFACE_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "ccnd/ccnd_private.h"

struct face* face_from_faceid(struct ccnd_handle *h, unsigned faceid);
void stuff_and_send(struct ccnd_handle* h, struct face* face, const unsigned char *data1, size_t size1, const unsigned char *data2, size_t size2, const char *tag, int lineno);

#ifdef __cplusplus
}

#include "facemgr.h"

namespace ndnfd {

// CcndFaceInterface moves messages between ccnd core and NDNFD face abstration.
class CcndFaceInterface : public Element {
 public:
  CcndFaceInterface(void) {}
  virtual ~CcndFaceInterface(void) {}
  
  void BindFaceThread(Ptr<FaceThread> face_thread);
  void BindFace(Ptr<Face> face);

  // Send sends a CCNB message through a face.
  void Send(FaceId faceid, const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2);
  
  // last message passed to process_input_message,
  // in case strategy wants to inspect (eg. to get incoming_sender)
  Ptr<Message> last_received_message_;
  
 private:
  // Receive is bound to face_thread.Receive port.
  // When a message is received, it is handed over to strategy.
  void Receive(Ptr<Message> message);

  DISALLOW_COPY_AND_ASSIGN(CcndFaceInterface);
};

};//namespace ndnfd
#endif
#endif//NDNFD_FACE_CCND_INTERFACE_H
