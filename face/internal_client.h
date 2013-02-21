#ifndef NDNFD_FACE_INTERNAL_CLIENT_H_
#define NDNFD_FACE_INTERNAL_CLIENT_H_
extern "C" {
#include <ccn/ccn_private.h>
#include "ccnd/ccnd_private.h"
}
#include "face/face.h"
#include "message/ccnb.h"
namespace ndnfd {

// An InternalClientFace talks with ccnd internal client.
class InternalClientFace : public Face {
 public:
  InternalClientFace(void) {}
  virtual void Init(void);
  virtual ~InternalClientFace(void);
  
  virtual bool CanSend(void) const { return true; }
  virtual bool CanReceive(void) const { return true; }

  virtual void Send(Ptr<Message> message);

  // Grab receives messages from internal client by reading its buffer.
  // This should be called in main loop.
  void Grab(void);
  
 private:
  ccn* internal_client(void) const { return const_cast<ccn*>(this->global()->ccndh()->internal_client); }
  DISALLOW_COPY_AND_ASSIGN(InternalClientFace);
};

/*
ccnd internal client is a standard ccn client.
* send to internal client: ccnd_send calls ccn_dispatch_message
* receive from internal client: process_internal_client_buffer calls ccn_grab_buffered_output

This may should be changed to operate on Messages directly. Benefits:
1. save a round of Encode-Decode
2. Message is tagged with incoming Face, so that we know whether to trust a face mgmt command.
   ccnd sets global variable h->interest_face before sending Interest to internal client,
   and inspects it when processing a command; this won't work with multi-threading.
   (it's also nice to use signature instead of incoming face to authentiate commands)

Current impl limitation:
* does not use WireProtocol
* sending requires message to be CcnbMessage
* receiving calls process_internal_client_buffer directly,
  so message goes into ccnd core without pushing to Receive port.

*/

};//namespace ndnfd
#endif//NDNFD_FACE_FACE_H
