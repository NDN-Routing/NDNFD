#ifndef NDNFD_MESSAGE_MESSAGE_H_
#define NDNFD_MESSAGE_MESSAGE_H_
#include "util/defs.h"
#include "message/messagebase.h"
#include "face/faceid.h"
namespace ndnfd {

//a logical unit that can be sent or received through a face
class Message : public MessageBase {
  public:
    static const MessageType kType = 1000;
    virtual MessageType type(void) const { return Message::kType; }

    FaceId incoming_face(void) const { return this->incoming_face_; }
    void set_incoming_face(FaceId value) { this->incoming_face_ = value; }
    const sockaddr_storage& incoming_sender(void) const { return this->incoming_sender_; }
    void set_incoming_sender(const sockaddr_storage value) { this->incoming_sender_ = value; }    

  private:
    FaceId incoming_face_;
    PeerAddress incoming_sender_;
    
    DISALLOW_COPY_AND_ASSIGN(Message);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_MESSAGE_H_
