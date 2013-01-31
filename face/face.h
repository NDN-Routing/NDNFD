#ifndef NDNFD_FACE_FACE_H_
#define NDNFD_FACE_FACE_H_
#include "face/channel.h"
#include "message/message.h"
namespace ndnfd {

enum FaceKind {
  kFKNone = 0,
  kFKInternal = 1,//talk to the internal client
  kFKApp = 2,//talk to a local application
  kFKMulticast = 3,//talk to multiple remote peers; a face receiving unicast packets from unknown peers also belongs to this kind, but that face cannot send
  kFKUnicast = 4//talk to one remote peer
};

enum FaceStatus {
  kFSNone = 0,
  kFSConnecting = 1,//connecting to remote peer
  kFSUndecided = 2,//accepted connection, no message received
  kFSEstablished = 3,//normal
  kFSClosing = 4//close after sending queued messages
};

class Face : public Element {
  public:
    Face(void);
    FaceId id(void) const { return this->id_; }
    FaceKind kind(void) const { return this->kind_; }
    FaceStatus status(void) const { return this->status_; }
    Ptr<Channel> channel(void) const { return this->channel_; }
    
    //whether this face may be used to send messages
    virtual bool CanSend(void) const { return true; }
    //send a message
    virtual void Send(Ptr<Message> message) { assert(false); }
    
    //whether this face may receive messages
    virtual bool CanReceive(void) const { return true; }
    PushFace<Ptr<Message>> Receive;
    
    //whether this face may accept new connection
    virtual bool CanAccept(void) const { return false; }
    PushFace<Ptr<Face>> Accept;
  
  protected:
    void set_id(FaceId value) { this->id_ = value; }
    void set_kind(FaceKind value) { this->kind_ = value; }
    void set_status(FaceStatus value) { this->status_ = value; }
    void set_channel(Ptr<Channel> value) { this->channel_ = value; }

  private:
    FaceId id_;
    FaceKind kind_;
    FaceStatus status_;
    Ptr<Channel> channel_;
    
    DISALLOW_COPY_AND_ASSIGN(Face);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACE_H
