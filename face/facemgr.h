#ifndef NDNFD_FACE_FACEMGR_H_
#define NDNFD_FACE_FACEMGR_H_
#include "face/face.h"
namespace ndnfd {

class FaceMgr : public Element {
  public:
    //get face by FaceId
    Ptr<Face> GetFace(FaceId id);

    //assign FaceId to a new face, and keep it in the table
    void AddFace(Ptr<Face> face);
    
    //remove face from table
    //called by a closing face after outbuf is sent
    void RemoveFace(Ptr<Face> face);
    
  private:
    std::unordered_map<FaceId,Ptr<Face>> faces_;
    DISALLOW_COPY_AND_ASSIGN(FaceMgr);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEMGR_H
