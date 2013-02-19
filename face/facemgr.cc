#include "facemgr.h"
#include "gtest/gtest.h"
namespace ndnfd {

FaceMgr::FaceMgr(void) {
  this->next_id_ = 0;
}

Ptr<Face> FaceMgr::GetFace(FaceId id) {
  auto it = this->table_.find(id);
  if (it == this->table_.end()) return nullptr;
  return it->second;
}

void FaceMgr::AddFace(Ptr<Face> face) {
  face->Enroll(++this->next_id_, this);
  this->table_[face->id()] = face;
}

void FaceMgr::RemoveFace(Ptr<Face> face) {
  this->table_.erase(face->id());
}

void FaceMgr::NotifyStatusChange(Ptr<Face> face) {
}

};//namespace ndnfd
