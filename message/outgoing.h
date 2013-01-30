#ifndef NDNFD_MESSAGE_OUTGOING_H_
#define NDNFD_MESSAGE_OUTGOING_H_
#include "util/defs.h"
#include "message/message.h"
namespace ndnfd {

//a message being sent through a face
template<typename TMessage>
class Outgoing {
  public:
    Outgoing(TMessage message, FaceId outgoing_face) { this->message_ = message; this->outgoing_face_ = outgoing_face; }

    MessageType type(void) const { return this->message_->type(); }
    TMessage message(void) const { return this->message_; }
    FaceId outgoing_face(void) const { return this->outgoing_face_; }
    
    //cancel sending
    //true if cancelled, false if already sending or sent
    bool CancelSend(void);
    
    //determine whether to send
    //true if should send, false if cancelled
    bool CanSend(void);
    
    //mark as sent
    void Sent(bool success);

  private:
    TMessage 
    FaceId outgoing_face_;
    
    DISALLOW_COPY_AND_ASSIGN(Outgoing);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_MESSAGE_H_
