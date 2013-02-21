#ifndef NDNFD_FACE_FACEMGR_H_
#define NDNFD_FACE_FACEMGR_H_
#include "face/face.h"
#include "face/ccnd_interface.h"
namespace ndnfd {

class CcndFaceInterface;

// A FaceMgr manages all Faces of a router.
class FaceMgr : public Element {
 public:
  FaceMgr(void);
  virtual void Init(void);
  virtual ~FaceMgr(void) {}
  Ptr<CcndFaceInterface> ccnd_face_interface(void) const { return this->ccnd_face_interface_; }
  
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
  FaceId next_id_;
  std::map<FaceId,Ptr<Face>> table_;
  Ptr<CcndFaceInterface> ccnd_face_interface_;

  DISALLOW_COPY_AND_ASSIGN(FaceMgr);
};

};//namespace ndnfd
#endif//NDNFD_FACE_FACEMGR_H
