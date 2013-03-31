#ifndef NDNFD_NS3_MODEL_APPFACE_H_
#define NDNFD_NS3_MODEL_APPFACE_H_
#include "face/face.h"
#include "l3protocol.h"
namespace ndnfd {

class SimAppFace : public Face {
 public:
  static const FaceType kType = 110;
  virtual FaceType type(void) const { return SimAppFace::kType; }

  explicit SimAppFace(Ptr<L3Protocol> l3) { this->l3_ = l3; }
  virtual void Init(void);
  virtual ~SimAppFace(void) {}

  virtual bool CanSend(void) const { return true; }
  virtual bool CanReceive(void) const { return true; }

  // Send delivers a message from NDNFD to App.
  virtual void Send(Ptr<const Message> message) { this->l3_->AppSend(this, PeekPointer(message)); }
  
  // Deliver delivers a message from App to NDNFD.
  void Deliver(Ptr<Message> message) { this->ReceiveMessage(message); }
  
 private:
  Ptr<L3Protocol> l3_;
  DISALLOW_COPY_AND_ASSIGN(SimAppFace);
};

};//namespace ndnfd
#endif//NDNFD_NS3_MODEL_APPFACE_H_
