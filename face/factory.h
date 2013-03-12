#ifndef NDNFD_FACE_FACTORY_H_
#define NDNFD_FACE_FACTORY_H_
#include "face/wireproto.h"
namespace ndnfd {

// A FaceFactory creates Face-related objects for a lower protocol.
class FaceFactory : public Element {
 public:
  virtual ~FaceFactory(void) {}

 protected:
  FaceFactory(void) { this->wp_ = nullptr; }
  FaceFactory(Ptr<WireProtocol> wp) { assert(wp != nullptr); this->wp_ = wp; }
  Ptr<WireProtocol> wp(void) const { return this->wp_; }

 private:
  Ptr<WireProtocol> wp_;

  DISALLOW_COPY_AND_ASSIGN(FaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACTORY_H
