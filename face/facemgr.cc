#include "facemgr.h"
#include "face/unix.h"
#include "face/ip.h"
#include "message/ccnb.h"
namespace ndnfd {

FaceMgr::FaceMgr(void) {
  this->next_id_ = 0;
  this->unix_factory_ = nullptr;
  this->unix_listener_ = nullptr;
  this->tcp_factory_ = nullptr;
  this->tcp_listener_ = nullptr;
  this->udp_factory_ = nullptr;
  this->udp_channel_ = nullptr;
}

void FaceMgr::Init(void) {
  this->ccnd_face_interface_ = this->New<CcndFaceInterface>();
}

FaceMgr::~FaceMgr(void) {
  this->set_unix_factory(nullptr);
  this->set_unix_listener(nullptr);
  this->set_tcp_factory(nullptr);
  this->set_tcp_listener(nullptr);
  this->set_udp_factory(nullptr);
  this->set_udp_channel(nullptr);
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
  this->ccnd_face_interface()->BindFace(face);
}

void FaceMgr::RemoveFace(Ptr<Face> face) {
  this->table_.erase(face->id());
}

void FaceMgr::NotifyStatusChange(Ptr<Face> face) {
}

void FaceMgr::MakeFactories(void) {
  this->set_unix_factory(this->New<UnixFaceFactory>(this->New<CcnbWireProtocol>(true)));
  this->set_tcp_factory(this->New<TcpFaceFactory>(this->New<CcnbWireProtocol>(true)));
  this->set_udp_factory(this->New<UdpFaceFactory>(this->New<CcnbWireProtocol>(false)));
}

void FaceMgr::StartDefaultListeners(void) {
  bool ok; NetworkAddress addr;

  this->set_unix_listener(this->unix_factory()->Listen("/tmp/.ccnd.sock"));

  std::tie(ok, addr) = IpAddressVerifier::Parse("0.0.0.0:9695");
  assert(ok);
  this->set_tcp_listener(this->tcp_factory()->Listen(addr));

  std::tie(ok, addr) = IpAddressVerifier::Parse("0.0.0.0:9695");
  assert(ok);
  this->set_udp_channel(this->udp_factory()->Channel(addr));
}

void FaceMgr::set_unix_factory(Ptr<UnixFaceFactory> value) {
  if (this->unix_factory_ != nullptr) {
    this->unix_factory_->Unref();
  }
  this->unix_factory_ = GetPointer(value);
}

void FaceMgr::set_unix_listener(Ptr<StreamListener> value) {
  if (this->unix_listener_ != nullptr) {
    this->unix_listener_->Unref();
  }
  this->unix_listener_ = GetPointer(value);
}

void FaceMgr::set_tcp_factory(Ptr<TcpFaceFactory> value) {
  if (this->tcp_factory_ != nullptr) {
    this->tcp_factory_->Unref();
  }
  this->tcp_factory_ = GetPointer(value);
}

void FaceMgr::set_tcp_listener(Ptr<StreamListener> value) {
  if (this->tcp_listener_ != nullptr) {
    this->tcp_listener_->Unref();
  }
  this->tcp_listener_ = GetPointer(value);
}

void FaceMgr::set_udp_factory(Ptr<UdpFaceFactory> value) {
  if (this->udp_factory_ != nullptr) {
    this->udp_factory_->Unref();
  }
  this->udp_factory_ = GetPointer(value);
}

void FaceMgr::set_udp_channel(Ptr<DgramChannel> value) {
  if (this->udp_channel_ != nullptr) {
    this->udp_channel_->Unref();
  }
  this->udp_channel_ = GetPointer(value);
}

Ptr<Face> FaceMgr::MakeUnicastFace(Ptr<Face> mcast_face, const NetworkAddress& peer) {
  // TODO don't assume mcast_face is DgramFace; it may be McastFace
  DgramFace* dface = static_cast<DgramFace*>(PeekPointer(mcast_face));
  Ptr<DgramChannel> channel = dface->channel();
  return channel->GetFace(peer);
}

std::tuple<bool,Ptr<Message>> FaceMgr::FaceMgmtRequest(const ccn_face_instance* req) {
  assert(false);//not implemented
  return std::forward_as_tuple(false, nullptr);
}

};//namespace ndnfd
