#ifndef NDNFD_MESSAGE_OUTGOING_H_
#define NDNFD_MESSAGE_OUTGOING_H_
#include "message/message.h"
namespace ndnfd {

// An Outgoing represents a message to be sent out through a Face,
template<typename TMessage>
class Outgoing {
 public:
  Outgoing(TMessage message, FaceId outgoing_face) { this->message_ = message; this->outgoing_face_ = outgoing_face; }

  MessageType type(void) const { return this->message_->type(); }
  TMessage message(void) const { return this->message_; }
  
  // Face through which this message will be sent
  FaceId outgoing_face(void) const { return this->outgoing_face_; }
  
  // CancelSend attempts to cancel sending, returns true if cancelled,
  // or false if the message is already sent.
  bool CancelSend(void);
  
  // CanSend is called before sending to determine whether sending should proceed.
  // It returns false if sending has been cancelled.
  bool CanSend(void) const;
  
  // Sent marks a message as sent.
  void Sent(bool success);

 private:
  TMessage message_;
  FaceId outgoing_face_;
  
  DISALLOW_COPY_AND_ASSIGN(Outgoing);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_OUTGOING_H_
