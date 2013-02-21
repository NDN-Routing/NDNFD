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
  FaceId id = 0;
  if (face->kind() != FaceKind::kInternal) id = ++this->next_id_;
  face->Enroll(id, this);
  this->table_[id] = face;
}

void FaceMgr::RemoveFace(Ptr<Face> face) {
  this->table_.erase(face->id());
}

void FaceMgr::NotifyStatusChange(Ptr<Face> face) {
}

};//namespace ndnfd
