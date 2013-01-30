#ifndef NDNFD_FACE_FACE_H_
#define NDNFD_FACE_FACE_H_
#include "face/channel.h"
#include "message/message.h"
namespace ndnfd {

enum FaceKind {
  kFKNone = 0,
  kFKInternal = 1,
  kFKApp = 2,
  kFKMulticast = 3,
  kFKUnicast = 4
};

enum FaceStatus {
  kFSNone = 0,
  kFSConnecting = 1,//connecting to remote peer
  kFSUndecided = 2,//accepted connection, no message received
  kFSEstablished = 3,//normal
  kFSClosing = 4//close after sending queued messages
};

class FaceMaster;

class Face : public Element {
  public:
    Face(void);
    FaceId id(void) const { return this->id_; }
    FaceKind kind(void) const { return this->kind_; }
    FaceStatus status(void) const { return this->status_; }
    Ptr<FaceMaster> master(void) const { return this->master_; }
    Ptr<Channel> channel(void) const { return this->channel_; }
    
    //whether this face may be used to send messages
    virtual bool CanSend(void) const { return true; }
    //send a message
    virtual void Send(Ptr<Message> message) =0;
    
    //whether this face may receive messages
    virtual bool CanReceive(void) const { return true; }
    PushFace<Ptr<Message>> Receive;
  
  protected:
    void set_id(FaceId value) { this->id_ = value; }
    void set_kind(FaceKind value) { this->kind_ = value; }
    void set_status(FaceStatus value) { this->status_ = value; }
    void set_master(Ptr<FaceMaster> value) { this->master_ = value; }
    void set_channel(Ptr<Channel> value) { this->channel_ = value; }

  private:
    FaceId id_;
    FaceKind kind_;
    FaceStatus status_;
    Ptr<FaceMaster> master_;
    Ptr<Channel> channel_;
    
    DISALLOW_COPY_AND_ASSIGN(Face);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACE_H
