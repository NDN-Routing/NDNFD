#ifndef NDNFD_FACE_FACEMGR_H_
#define NDNFD_FACE_FACEMGR_H_
#include "face/face.h"
namespace ndnfd {

// A FaceMgr manages all Faces of a router.
class FaceMgr : public Element {
  public:
    // GetFace finds a Face by FaceId.
    Ptr<Face> GetFace(FaceId id);
    
    // AddFace assigns FaceId to a new Face, and puts it in the table.
    void AddFace(Ptr<Face> face);
    
    // RemoveFace removes Face from the table.
    // It is called by a closing Face after send queue is empty.
    void RemoveFace(Ptr<Face> face);
    
    // NotifyStatusChange is called by Face when status is changed.
    void NotifyStatusChange(Ptr<Face> face);
    
  private:
    std::unordered_map<FaceId,Ptr<Face>> faces_;
    DISALLOW_COPY_AND_ASSIGN(FaceMgr);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEMGR_H
