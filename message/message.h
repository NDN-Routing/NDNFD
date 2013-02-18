#ifndef NDNFD_MESSAGE_MESSAGE_H_
#define NDNFD_MESSAGE_MESSAGE_H_
#include "message/messagebase.h"
#include "face/faceid.h"
namespace ndnfd {

// A Message is a logical unit that can be sent or received through a Face.
class Message : public MessageBase {
 public:
  static const MessageType kType = 1000;
  virtual ~Message(void) {}
  virtual MessageType type(void) const { return Message::kType; }

  // Face through which this Message enters the router
  FaceId incoming_face(void) const { return this->incoming_face_; }
  void set_incoming_face(FaceId value) { this->incoming_face_ = value; }

  // remote peer that sends this Message
  // Address format is defined by the incoming Face.
  const NetworkAddress& incoming_sender(void) const { return this->incoming_sender_; }
  void set_incoming_sender(const NetworkAddress& value) { this->incoming_sender_ = value; }  

 protected:
  Message(void) {}

 private:
  FaceId incoming_face_;
  NetworkAddress incoming_sender_;
  
  DISALLOW_COPY_AND_ASSIGN(Message);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_MESSAGE_H_
