#include "facemgr.h"
#include "face/unix.h"
#include "face/ip.h"
#include "face/ether.h"
#include "message/ccnb.h"
#include "face/ndnlp.h"
namespace ndnfd {

FaceMgr::FaceMgr(void) {
  this->next_id_ = 0;
  this->unix_listener_ = nullptr;
  this->tcp_factory_ = nullptr;
  this->tcp_listener_ = nullptr;
  this->udp_channel_ = nullptr;
  this->udp_ndnlp_channel_ = nullptr;
}

void FaceMgr::Init(void) {
  this->ccnd_face_interface_ = this->New<CcndFaceInterface>();
}

FaceMgr::~FaceMgr(void) {
  this->set_unix_listener(nullptr);
  this->set_tcp_factory(nullptr);
  this->set_tcp_listener(nullptr);
  this->set_udp_channel(nullptr);
  this->set_udp_ndnlp_channel(nullptr);

  while (!this->ether_channels_.empty()) {
    std::string ifname; DgramChannel* channel; DgramFace* face;
    std::tie(ifname, channel, face) = this->ether_channels_.back();
    if (face != nullptr) face->Unref();
    if (channel != nullptr) channel->Unref();
    this->ether_channels_.pop_back();
  }
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
  // TODO finalize_face, content_queue_destroy
}

void FaceMgr::NotifyStatusChange(Ptr<Face> face) {
  // TODO call ccnd_face_status_change if necessary
}

void FaceMgr::StartDefaultListeners(void) {
  bool ok; NetworkAddress addr;

  Ptr<UnixFaceFactory> unix_factory = this->New<UnixFaceFactory>(this->New<CcnbWireProtocol>(true));
  this->set_unix_listener(unix_factory->Listen("/tmp/.ccnd.sock"));

  Ptr<TcpFaceFactory> tcp_factory = this->New<TcpFaceFactory>(this->New<CcnbWireProtocol>(true));
  this->set_tcp_factory(tcp_factory);
  std::tie(ok, addr) = IpAddressVerifier::Parse("0.0.0.0:9695"); assert(ok);
  this->set_tcp_listener(this->tcp_factory()->Listen(addr));
  
  Ptr<UdpFaceFactory> udp_factory = this->New<UdpFaceFactory>(this->New<CcnbWireProtocol>(false));
  std::tie(ok, addr) = IpAddressVerifier::Parse("0.0.0.0:9695"); assert(ok);
  this->set_udp_channel(udp_factory->Channel(addr));

  std::tie(ok, addr) = IpAddressVerifier::Parse("192.168.3.1:59695"); assert(ok);
  NetworkAddress group_addr; std::tie(ok, group_addr) = IpAddressVerifier::Parse("224.0.23.170:59695"); assert(ok);
  // TODO one mcast face per interface
  this->set_udp_mcast_face(udp_factory->McastFace(addr, group_addr, 1));
  
  Ptr<UdpFaceFactory> udp_ndnlp_factory = this->New<UdpFaceFactory>(this->New<NdnlpWireProtocol>(1460));
  std::tie(ok, addr) = IpAddressVerifier::Parse("0.0.0.0:29695"); assert(ok);
  this->set_udp_ndnlp_channel(udp_ndnlp_factory->Channel(addr));
  
  std::tie(ok, addr) = EtherAddressVerifier::Parse("01:00:5E:00:17:AA"); assert(ok);
  Ptr<EtherFaceFactory> ether_factory = this->New<EtherFaceFactory>();
  for (std::string ifname : ether_factory->ListNICs()) {
    Ptr<DgramChannel> channel = ether_factory->Channel(ifname, 0x8624);
    if (channel != nullptr) {
      Ptr<DgramFace> mcast_face = channel->GetMcastFace(addr);
      this->ether_channels().emplace_back(ifname, GetPointer(channel), GetPointer(mcast_face));
    }
  }
}

#define FACEMGR_DEF_SETTER(field,type) \
void FaceMgr::set_##field(Ptr<type> value) { \
  if (this->field##_ != nullptr) { \
    this->field##_->Unref(); \
  } \
  this->field##_ = GetPointer(value); \
}
FACEMGR_DEF_SETTER(unix_listener, StreamListener);
FACEMGR_DEF_SETTER(tcp_factory, TcpFaceFactory);
FACEMGR_DEF_SETTER(tcp_listener, StreamListener);
FACEMGR_DEF_SETTER(udp_channel, DgramChannel);
FACEMGR_DEF_SETTER(udp_mcast_face, DgramFace);
FACEMGR_DEF_SETTER(udp_ndnlp_channel, DgramChannel);

Ptr<Face> FaceMgr::MakeUnicastFace(Ptr<Face> mcast_face, const NetworkAddress& peer) {
  DgramFace* dface = static_cast<DgramFace*>(PeekPointer(mcast_face));
  Ptr<DgramChannel> channel = dface->channel();
  return channel->GetFace(peer);
}

std::tuple<bool,Ptr<Message>> FaceMgr::FaceMgmtRequest(const ccn_face_instance* req) {
  assert(false);//not implemented
  return std::forward_as_tuple(false, nullptr);
}

};//namespace ndnfd
