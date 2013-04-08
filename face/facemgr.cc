#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "facemgr.h"
#include "face/unix.h"
#include "face/ip.h"
#include "face/ether.h"
#include "message/ccnb.h"
#include "message/contentobject.h"
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
  
  while (!this->udp_mcast_faces_.empty()) {
    DgramFace* face = this->udp_mcast_faces_.back();
    face->Unref();
    this->udp_mcast_faces_.pop_back();
  }

  while (!this->ether_channels_.empty()) {
    std::string ifname; DgramChannel* channel; DgramFace* face;
    std::tie(ifname, channel, face) = this->ether_channels_.back();
    if (face != nullptr) face->Unref();
    if (channel != nullptr) channel->Unref();
    this->ether_channels_.pop_back();
  }
}

Ptr<Face> FaceMgr::GetFace(FaceId id) const {
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
  face->Finalize();
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

  //std::tie(ok, addr) = IpAddressVerifier::Parse("224.0.23.170:59695"); assert(ok);
  //for (NetworkAddress local_addr : udp_factory->ListLocalAddresses()) {
  //  reinterpret_cast<sockaddr_in*>(&local_addr.who)->sin_port = htobe16(59695);
  //  Ptr<DgramFace> mcast_face = udp_factory->McastFace(local_addr, addr, 1);
  //  if (mcast_face != nullptr) this->add_udp_mcast_face(mcast_face);
  //}
  
  //Ptr<UdpFaceFactory> udp_ndnlp_factory = this->New<UdpFaceFactory>(this->New<NdnlpWireProtocol>(1460));
  //std::tie(ok, addr) = IpAddressVerifier::Parse("0.0.0.0:29695"); assert(ok);
  //this->set_udp_ndnlp_channel(udp_ndnlp_factory->Channel(addr));
  
  std::tie(ok, addr) = EtherAddressVerifier::Parse("01:00:5E:00:17:AA"); assert(ok);
  Ptr<EtherFaceFactory> ether_factory = this->New<EtherFaceFactory>();
  for (std::string ifname : ether_factory->ListNICs()) {
    Ptr<DgramChannel> channel = ether_factory->Channel(ifname, 0x8624);
    if (channel != nullptr) {
      Ptr<DgramFace> mcast_face = channel->GetMcastFace(addr);
      this->add_ether_channel(ifname, channel, mcast_face);
    }
  }
}

DgramChannel* FaceMgr::ether_channel(std::string ifname) const {
  for (auto ether_tuple : this->ether_channels()) {
    if (std::get<0>(ether_tuple) == ifname) return std::get<1>(ether_tuple);
  }
  return nullptr;
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
FACEMGR_DEF_SETTER(udp_ndnlp_channel, DgramChannel);

void FaceMgr::add_udp_mcast_face(Ptr<DgramFace> mcast_face) {
  this->udp_mcast_faces_.push_back(GetPointer(mcast_face));
}

void FaceMgr::add_ether_channel(const std::string& ifname, Ptr<DgramChannel> channel, Ptr<DgramFace> mcast_face) {
  this->ether_channels_.emplace_back(ifname, GetPointer(channel), GetPointer(mcast_face));
}

Ptr<Face> FaceMgr::MakeUnicastFace(Ptr<Face> mcast_face, const NetworkAddress& peer) {
  assert(mcast_face->kind() == FaceKind::kMulticast);
  assert(DgramFace::IsDgramFaceType(mcast_face->type()));
  
  DgramFace* dface = static_cast<DgramFace*>(PeekPointer(mcast_face));
  Ptr<DgramChannel> channel = dface->channel();
  Ptr<Face> face = channel->GetFace(peer);
  assert(face != nullptr && face->kind() == FaceKind::kUnicast);
  return face;
}

std::tuple<InternalClientHandler::ResponseKind,Ptr<Buffer>> FaceMgr::FaceMgmtReq(FaceMgmtProtoAct act, FaceId inface, const uint8_t* msg, size_t size) {
  // decode msg as face_instance
  Ptr<ContentObjectMessage> pco = ContentObjectMessage::Parse(msg, size);
  if (pco == nullptr)
    return std::forward_as_tuple(InternalClientHandler::ResponseKind::kSilent, nullptr);
  const uint8_t *req;
  size_t req_size;
  std::tie(req, req_size) = pco->payload();
  struct ccn_face_instance *face_instance = nullptr;
  face_instance = ccn_face_instance_parse(req, req_size);
  if (face_instance == nullptr || face_instance->action == nullptr)
    return std::forward_as_tuple(InternalClientHandler::ResponseKind::kSilent, nullptr);
  // verify action in msg matches act
  switch (act) {
    case FaceMgmtProtoAct::kNewFace:
      if (strcmp(face_instance->action, "newface") != 0) {
        return std::forward_as_tuple(InternalClientHandler::ResponseKind::kSilent, nullptr);
      }
      break;
    case FaceMgmtProtoAct::kDestroyFace:
      if (strcmp(face_instance->action, "destroyface") != 0) {
        return std::forward_as_tuple(InternalClientHandler::ResponseKind::kSilent, nullptr);
      }
      break;
    default: assert(false); break;
  }
  // verify inface trusted for face mgmt protocol
  if (this->GetFace(inface)->kind() != FaceKind::kApp) {
    return std::forward_as_tuple(InternalClientHandler::ResponseKind::kSilent, nullptr);
  }
  bool ok; int errnum; std::string errmsg;
  switch (act) {
    case FaceMgmtProtoAct::kNewFace:
      std::tie(ok, errnum, errmsg) = this->FaceMgmtNewFace(face_instance);
      break;
    case FaceMgmtProtoAct::kDestroyFace:
      std::tie(ok, errnum, errmsg) = this->FaceMgmtDestroyFace(face_instance);
      break;
    default: assert(false); break;
  }
  if (!ok) {
    // return NACK with errnum and errmsg
    struct ccn_charbuf* cb_msg = ccn_charbuf_create();
    cb_msg->length = 0;
    ccn_encode_StatusResponse(cb_msg, errnum, errmsg.c_str());
    Ptr<Buffer> b1 = new Buffer(0);
    b1->Swap(cb_msg);
    ccn_charbuf_destroy(&cb_msg);
    return std::forward_as_tuple(InternalClientHandler::ResponseKind::kNack, b1);
  }
  face_instance->ccnd_id = CCNDH->ccnd_id;
  struct ccn_charbuf* cb_face_inst = ccn_charbuf_create();
  ccnb_append_face_instance(cb_face_inst, face_instance);
  Ptr<Buffer> b = new Buffer(0);
  b->Swap(cb_face_inst);
  ccn_charbuf_destroy(&cb_face_inst);
  ccn_face_instance_destroy(&face_instance);
  return std::forward_as_tuple(InternalClientHandler::ResponseKind::kRespond, b);
}

std::tuple<bool,int,std::string> FaceMgr::FaceMgmtNewFace(ccn_face_instance* face_inst) {
  if (face_inst->descr.address == nullptr) {
    return std::forward_as_tuple(false, 504, "parameter error");
  }
  Ptr<Face> face = nullptr;
  int errnum;
  std::string errmsg;
  switch (face_inst->descr.ipproto) {
    case IPPROTO_UDP:
    case IPPROTO_TCP: {
      std::tie(face, errnum, errmsg) = this->FaceMgmtNewIpFace(face_inst);
    }
    break;
    case FaceMgr::kFaceMgmtProto_Ether: {
      std::tie(face, errnum, errmsg) = this->FaceMgmtNewEtherFace(face_inst);
    }
    break;
    default: {
      return std::forward_as_tuple(false, 504, "parameter error");
    }
  }
  if (face == nullptr) {
    return std::forward_as_tuple(false, errnum, errmsg);
  } else {
    face_inst->faceid = static_cast<unsigned>(face->id());
    return std::forward_as_tuple(true, 0, "");
  }
}

std::tuple<Ptr<Face>,int,std::string> FaceMgr::FaceMgmtNewIpFace(const ccn_face_instance* face_inst) {
  struct addrinfo hints = {0};
  struct addrinfo *addrinfo = nullptr;
  hints.ai_flags |= AI_NUMERICHOST;
  hints.ai_protocol = face_inst->descr.ipproto;
  hints.ai_socktype = (hints.ai_protocol == IPPROTO_UDP) ? SOCK_DGRAM : SOCK_STREAM;
  int res = getaddrinfo(face_inst->descr.address, face_inst->descr.port, &hints, &addrinfo);
  if (res != 0) {
    return std::forward_as_tuple(nullptr, res, "error: getaddrinfo");
  }
  NetworkAddress addr;
  switch (addrinfo->ai_family) {
    case AF_INET: {
      addr.wholen = sizeof(sockaddr_in);//or sockaddr_in6 for IPv6
      sockaddr_in* sa = reinterpret_cast<sockaddr_in*>(&addr.who);
      sa->sin_family = AF_INET;
      sa->sin_port = reinterpret_cast<sockaddr_in*>(addrinfo->ai_addr)->sin_port;
      sa->sin_addr.s_addr = reinterpret_cast<sockaddr_in*>(addrinfo->ai_addr)->sin_addr.s_addr;
    } 
    break;
    case AF_INET6: {
      addr.wholen = sizeof(sockaddr_in6);
      sockaddr_in6* sa = reinterpret_cast<sockaddr_in6*>(&addr.who);
      sa->sin6_family = AF_INET6;
      sa->sin6_port = reinterpret_cast<sockaddr_in6*>(addrinfo->ai_addr)->sin6_port;
      memcpy(sa->sin6_addr.s6_addr, reinterpret_cast<sockaddr_in6*>(addrinfo->ai_addr)->sin6_addr.s6_addr, sizeof(sa->sin6_addr.s6_addr));
    }
    break;
    default: {
      return std::forward_as_tuple(nullptr, 0, "unknown address type");
    }
    break;
  }
  switch (face_inst->descr.ipproto) {
    case IPPROTO_TCP: {
      //   TCP: this->tcp_factory()->Connect(addr)
      Ptr<Face> face = this->tcp_factory()->Connect(addr);
      return std::forward_as_tuple(face, 0, "");
    }
    break;
    case IPPROTO_UDP: {
      if (IpAddressVerifier::IsMcast(addr)) {
        //  UDP multicast: fail, not supported
        return std::forward_as_tuple(nullptr, 0, "UDP Multicast is not Supported");
      } else {
        //   UDP unicast: this->udp_channel()->GetFace(addr)
        Ptr<Face> face = this->udp_channel()->GetFace(addr);
        return std::forward_as_tuple(face, 0, "");
      }
    }
    break;
    default: {
      return std::forward_as_tuple(nullptr, 0, "unknown address type");
    }
  }
}

std::tuple<Ptr<Face>,int,std::string> FaceMgr::FaceMgmtNewEtherFace(const ccn_face_instance* face_inst) {
  bool ok;
  NetworkAddress addr;
  std::tie(ok, addr) = EtherAddressVerifier::Parse(face_inst->descr.address);
  if (ok) {
    std::string sourceAddress = face_inst->descr.source_address;
    std::string prefix = "localif=";
    std::string ifname;
    if (sourceAddress.substr(0, prefix.size()) == prefix) {
      ifname = sourceAddress.substr(prefix.size());
    } else {
      return std::forward_as_tuple(nullptr, -1, "ethernet interface does not begin with 'localif='");
    }
    DgramChannel* etherChannel = this->ether_channel(ifname);
    if (etherChannel == nullptr) {
      return std::forward_as_tuple(nullptr, -1, "ether_channel is null.");
    }
    Ptr<Face> face = etherChannel->GetFace(addr);
    return std::forward_as_tuple(face, 0, "");
  } else {
    return std::forward_as_tuple(nullptr, -1, "fail to parse ethernet address");
  }
}

std::tuple<bool,int,std::string> FaceMgr::FaceMgmtDestroyFace(ccn_face_instance* face_inst) {
  // face = this->GetFace(faceid)
  Ptr<Face> face = this->GetFace(face_inst->faceid);
  // fail if face == nullptr
  if (face == nullptr) {
    return std::forward_as_tuple(false, 0, "");
  }
  face->Close();
  return std::forward_as_tuple(true, 0, "");
}


};//namespace ndnfd
