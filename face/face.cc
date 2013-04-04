#include "face.h"
#include "facemgr.h"
extern "C" {
void finalize_face(struct ccnd_handle* h, struct face* face);
}
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
  memset(&this->ccnd_face_, 0, sizeof(this->ccnd_face_));
}

void Face::Enroll(FaceId id, Ptr<FaceMgr> mgr) {
  assert(mgr == this->global()->facemgr());
  assert(this->id_ == FaceId_none);

  this->set_id(id);
  // update ccnd_face, like ccnd_enroll_face
  this->ccnd_face()->meter[FM_BYTI] = ccnd_meter_create(CCNDH, "bytein");
  this->ccnd_face()->meter[FM_BYTO] = ccnd_meter_create(CCNDH, "byteout");
  this->ccnd_face()->meter[FM_INTI] = ccnd_meter_create(CCNDH, "intrin");
  this->ccnd_face()->meter[FM_INTO] = ccnd_meter_create(CCNDH, "introut");
  this->ccnd_face()->meter[FM_DATI] = ccnd_meter_create(CCNDH, "datain");
  this->ccnd_face()->meter[FM_DATO] = ccnd_meter_create(CCNDH, "dataout");
}

void Face::Finalize(void) {
  if (this->status_ == FaceStatus::kFinalized) return;
  this->DoFinalize();
  finalize_face(CCNDH, this->ccnd_face());
  this->status_ = FaceStatus::kFinalized;
}

void Face::set_id(FaceId value) {
  this->id_ = value;
  //this->Log(kLLInfo, kLCFace, "Face(%"PRIxPTR")::set_id(%"PRI_FaceId")", this, this->id());
  this->ccnd_face()->faceid = value == FaceId_none ? CCN_NOFACEID : static_cast<unsigned>(value);
}

void Face::set_kind(FaceKind value) {
  this->kind_ = value;
  
  const int ccnd_flags_mask = CCN_FACE_GG | CCN_FACE_LOCAL | CCN_FACE_MCAST | CCN_FACE_LOOPBACK;
  int ccnd_flags = 0;
  switch (value) {
    case FaceKind::kInternal:  ccnd_flags = CCN_FACE_GG | CCN_FACE_LOCAL; break;
    case FaceKind::kApp:       ccnd_flags = CCN_FACE_GG;                  break;
    case FaceKind::kMulticast: ccnd_flags = CCN_FACE_MCAST;               break;
    default: break;
  }
  this->set_ccnd_flags(ccnd_flags, ccnd_flags_mask);
}

void Face::set_status(FaceStatus value) {
  assert(value != FaceStatus::kFinalized);
  FaceStatus old_status = this->status_;
  if (old_status == value || old_status == FaceStatus::kFinalized) return;
  this->status_ = value;

  // let process_input_message unset UNDECIDED and call register_new_face
  int ccnd_flags_mask = CCN_FACE_CONNECTING | CCN_FACE_CLOSING;
  int ccnd_flags = 0;
  switch (value) {
    case FaceStatus::kConnecting: ccnd_flags = CCN_FACE_CONNECTING; break;
    case FaceStatus::kUndecided:  ccnd_flags = CCN_FACE_UNDECIDED; 
                             ccnd_flags_mask |= CCN_FACE_UNDECIDED; break;
    case FaceStatus::kClosing:    ccnd_flags = CCN_FACE_CLOSING;    break;
    default: break;
  }
  this->set_ccnd_flags(ccnd_flags, ccnd_flags_mask);
  
  if (old_status != FaceStatus::kNone) {
    this->Log(FaceStatus_IsError(value)?kLLWarn:kLLInfo, kLCFace, "Face(%" PRIxPTR ",%" PRI_FaceId ")::set_status %s=>%s", this, this->id(), FaceStatus_ToString(old_status).c_str(), FaceStatus_ToString(value).c_str());
  }
  if (this->id() != FaceId_none) {
    this->global()->facemgr()->NotifyStatusChange(this);
    if (value == FaceStatus::kClosed || FaceStatus_IsError(value)) {
      this->global()->facemgr()->RemoveFace(this);
    }
  }
}

void Face::ReceiveMessage(Ptr<Message> msg) {
  assert(msg != nullptr);
  msg->set_incoming_face(this->id());
  this->Receive(msg);
}

};//namespace ndnfd
