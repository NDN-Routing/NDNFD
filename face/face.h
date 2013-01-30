#ifndef CCND2_FACE_FACE_H_
#define CCND2_FACE_FACE_H_
#include "util/defs.h"
#include "message/message.h"
namespace ccnd2 {

enum FaceType {
  kFTNone = 0,
  kFTInternal = 1,
  kFTApp = 2,
  kFTMulticast = 3,
  kFTUnicast = 4
};

enum FaceStatus {
  kFSNone = 0,
  kFSConnecting = 1,//connecting to remote peer
  kFSUndecided = 2,//accepted connection, no message received
  kFSEstablished = 3,//normal
  kFSClosing = 4//close after sending queued messages
};

class FaceMaster;

class Face : public Object {
  public:
    typedef boost::function1<void,Ptr<Message>> ReceiveCb;
    
    FaceId id(void) const { return this->id_; }
    FaceType type(void) const { return this->type_; }
    FaceStatus status(void) const { return this->status_; }
    Ptr<FaceMaster> master(void) const { return this->master_; }
    
    //whether this face may be used to send messages
    virtual bool CanSend(void) const { return true; }
    //send a message
    virtual void Send(Ptr<Message> message) =0;
    
    //whether this face may receive messages
    virtual bool CanReceive(void) const { return true; }
    //register a callback for received messages
    void set_receive_cb(ReceiveCb value) { this->receive_cb_ = value; }

    virtual void Receive(Ptr<FaceMaster> message) { this->call_receive_cb(message); }
  
  protected:
    void set_id(FaceId value) { this->id_ = value; }
    void set_type(FaceType value) { this->type_ = value; }
    void set_status(FaceStatus value) { this->status_ = value; }
    void set_master(Ptr<FaceMaster> value) { this->master_ = value; }

    void call_receive_cb(Ptr<Message> message) { if (this->receive_cb() != NULL) this->receive_cb()(message); }

  private:
    FaceId id_;
    FaceType type_;
    FaceStatus status_;
    boost::Ptr<FaceMaster> master_;
    ReceiveCb receive_cb_;

    ReceiveCb receive_cb(void) const { return this->receive_cb_; }
    
    DISALLOW_COPY_AND_ASSIGN(Face);
};

};//namespace ccnd2
#endif//CCND2_FACE_FACE_H
