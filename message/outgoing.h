#ifndef NDNFD_MESSAGE_OUTGOING_H_
#define NDNFD_MESSAGE_OUTGOING_H_
#include "message/message.h"
namespace ndnfd {

// An Outgoing represents a message to be sent out through a Face.
// This is not currently used because send_queue is provided by ccnd core.
class Outgoing : public Object {
 public:
  Outgoing(Ptr<Message> message, FaceId outgoing_face);
  virtual ~Outgoing(void) {}

  MessageType type(void) const { return this->message_->type(); }
  Ptr<Message> message(void) const { return this->message_; }
  
  // Face through which this message will be sent
  FaceId outgoing_face(void) const { return this->outgoing_face_; }
  
  // Cancel attempts to cancel sending, returns true if cancelled,
  // or false if the message is already sent.
  bool Cancel(void);
  
  // Sending determines whether sending should proceed, and marks as sent.
  // It returns false if cancelled or sent.
  bool Sending(void);

 private:
  Ptr<Message> message_;
  FaceId outgoing_face_;
  std::atomic_flag lock_;
  bool cancelled_;
  bool sent_;
  
  DISALLOW_COPY_AND_ASSIGN(Outgoing);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_OUTGOING_H_
