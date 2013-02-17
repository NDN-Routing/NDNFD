#ifndef NDNFD_FACE_FACTORY_H_
#define NDNFD_FACE_FACTORY_H_
#include "face/wireproto.h"
namespace ndnfd {

// A FaceFactory creates Face-related objects for a lower protocol.
class FaceFactory : public Element {
 public:
  FaceFactory(Ptr<WireProtocol> wp);

 protected:
  Ptr<WireProtocol> wp(void) const { return this->wp_; }

 private:
  Ptr<WireProtocol> wp_;

  DISALLOW_COPY_AND_ASSIGN(FaceFactory);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACTORY_H
