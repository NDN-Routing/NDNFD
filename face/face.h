#ifndef NDNFD_FACE_FACE_H_
#define NDNFD_FACE_FACE_H_
#include "core/element.h"
#include "message/message.h"
namespace ndnfd {

// FaceKind describes what's the peer(s) of a Face.
enum class FaceKind {
  kNone    = 0,
  kInternal  = 1,//talk to the internal client
  kApp     = 2,//talk to a local application
  kMulticast = 3,//talk to multiple remote peers; a face receiving unicast packets from unknown peers also belongs to this kind, but that face cannot send
  kUnicast   = 4 //talk to one remote peer
};

// FaceStatus describes the status of a Face.
enum class FaceStatus {
  kNone    = 0,
  kConnecting  = 1,//connecting to remote peer
  kUndecided   = 2,//accepted connection, no message received
  kEstablished = 3,//normal
  kClosing   = 4,//close after sending queued messages
  kError     = 5 //error, no longer usable
};


// A Face is a logical connection to a local entity, or one or more remote peers.
class Face : public Element {
 public:
  Face(void);
  FaceId id(void) const { return this->id_; }
  FaceKind kind(void) const { return this->kind_; }
  FaceStatus status(void) const { return this->status_; }

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
  
  // Enroll verifies mgr is the FaceMgr of this router,
  // and records the assigned FaceId.
  virtual void Enroll(FaceId id, Ptr<FaceMgr> mgr);
  
 protected:
  void set_id(FaceId value) { this->id_ = value; }
  void set_kind(FaceKind value) { this->kind_ = value; }
  void set_status(FaceStatus value);//should notify FaceMgr

 private:
  FaceId id_;
  FaceKind kind_;
  FaceStatus status_;
  
  DISALLOW_COPY_AND_ASSIGN(Face);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACE_H
