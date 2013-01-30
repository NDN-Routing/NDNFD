#ifndef NDNFD_FACE_FACEMGR_H_
#define NDNFD_FACE_FACEMGR_H_
#include "util/defs.h"
#include "face/face.h"
namespace ndnfd {

class FaceMgr : public Object {
  public:
    Ptr<Face> GetFace(FaceId id);
    FaceId AssignFaceId(void);
    
  private:
    DISALLOW_COPY_AND_ASSIGN(FaceMgr);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACE_H
