#include "face.h"
namespace ndnfd {

std::string FaceKind_ToString(FaceKind kind) {
  switch (kind) {
    case FaceKind::kInternal:  return "Internal";
    case FaceKind::kApp:       return "App";
    case FaceKind::kMulticast: return "Multicast";
    case FaceKind::kUnicast:   return "Unicast";
    default:                   return "None";
  }
}

std::string FaceStatus_ToString(FaceStatus status) {
  switch (status) {
    case FaceStatus::kConnecting:    return "Connecting";
    case FaceStatus::kUndecided:     return "Undecided";
    case FaceStatus::kEstablished:   return "Established";
    case FaceStatus::kClosing:       return "Closing";
    case FaceStatus::kClosed:        return "Closed";
    case FaceStatus::kConnectError:  return "ConnectError";
    case FaceStatus::kProtocolError: return "ProtocolError";
    case FaceStatus::kDisconnect:    return "Disconnect";
    default:                         return "None";
  }
}

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
