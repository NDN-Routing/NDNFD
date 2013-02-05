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


// A Face is a logical connection to a local entity, or one or more remote peers.
class Face : public Element {
  public:
    Face(void);
    FaceId id(void) const { return this->id_; }
    FaceKind kind(void) const { return this->kind_; }
    FaceStatus status(void) const { return this->status_; }

    //called by FaceMgr
    void set_id(FaceId value) { this->id_ = value; }
    
    // CanSend returns true if this Face may be used to send messages.
    virtual bool CanSend(void) const { return false; }
    // Send enqueues a message for sending.
    virtual void Send(Ptr<Message> message) { assert(false); }
    
    // CanReceive returns true if this Face may be used to receive messages.
    virtual bool CanReceive(void) const { return false; }
    // Receive is called when a message is received.
    PushFace<Ptr<Message>> Receive;
    
    // CanAccept returns true if this Face may accept new connection.
    virtual bool CanAccept(void) const { return false; }
    // Accept is called when a new connection is accepted as a new Face.
    PushFace<Ptr<Face>> Accept;
  
  protected:
    void set_kind(FaceKind value) { this->kind_ = value; }
    void set_status(FaceStatus value) { this->status_ = value; }

  private:
    FaceId id_;
    FaceKind kind_;
    FaceStatus status_;
    
    DISALLOW_COPY_AND_ASSIGN(Face);
};

// An IAddressVerifier implementor knows about the address format of a lower protocol,
// such as IPv4 or Ethernet.
class IAddressVerifier {
  public:
    // CheckAddress checks whether addr is valid in lower protocol.
    virtual bool CheckAddress(const NetworkAddress& addr) =0;
    
    // NormalizeAddress clears certains fields in addr so that it is suitable to use as a hash key.
    virtual void NormalizeAddress(NetworkAddress& addr) {}
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACE_H
