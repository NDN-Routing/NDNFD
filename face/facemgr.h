#ifndef CCND2_FACE_FACEMGR_H_
#define CCND2_FACE_FACEMGR_H_
#include "util/defs.h"
#include "face/face.h"
namespace ccnd2 {

class FaceMgr : public Object {
  public:
    Ptr<Face> GetFace(FaceId id);
    FaceId AssignFaceId(void);
    
  private:
    DISALLOW_COPY_AND_ASSIGN(FaceMgr);
};

};//namespace ccnd2
#endif//CCND2_FACE_FACE_H
