#include "face.h"
namespace ndnfd {

bool FaceStatus_IsError(FaceStatus status) {
  switch (status) {
    case FaceStatus::kConnectError:
    case FaceStatus::kProtocolError:
    case FaceStatus::kDisconnect:
      return true;
    default:
      return false;
  }
}

bool FaceStatus_IsUsable(FaceStatus status) {
  switch (status) {
    case FaceStatus::kConnecting:
    case FaceStatus::kUndecided:
    case FaceStatus::kEstablished:
      return true;
    default:
      return false;
  }
}

Face::Face(void) {
  this->id_ = FaceId_none;
  this->kind_ = FaceKind::kNone;
  this->status_ = FaceStatus::kNone;
}

void Face::Enroll(FaceId id, Ptr<FaceMgr> mgr) {
  assert(mgr == this->global()->facemgr());
  this->set_id(id);
}

void Face::set_status(FaceStatus value) {
  // TODO notify FaceMgr
  this->status_ = value;
}

};//namespace ndnfd
