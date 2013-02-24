#include "dgram.h"
#include "face/facemgr.h"
namespace ndnfd {

DgramFace::DgramFace(Ptr<DgramChannel> channel, const NetworkAddress& peer) {
  assert(channel != nullptr);
  this->channel_ = channel;
  this->peer_ = peer;
  this->set_status(FaceStatus::kUndecided);
  this->set_ccnd_flags(CCN_FACE_DGRAM | CCN_FACE_INET, CCN_FACE_DGRAM | CCN_FACE_INET | CCN_FACE_INET6);//INET or INET6 doesn't matter
}

void DgramFace::Init(void) {
  this->global()->facemgr()->AddFace(this);
}

void DgramFace::Send(Ptr<Message> message) {
  if (!this->CanSend()) {
    this->Log(kLLError, kLCFace, "DgramFace(%"PRI_FaceId")::Send !CanSend", this->id());
    return;
  }
  this->channel()->FaceSend(this, message);
}

void DgramFace::Deliver(Ptr<Message> msg) {
  if (this->status() == FaceStatus::kUndecided) this->set_status(FaceStatus::kEstablished);
  this->ReceiveMessage(msg);
}

void DgramFace::Close(void) {
  this->set_status(FaceStatus::kClosed);
  this->channel_ = nullptr;
}

DgramFallbackFace::DgramFallbackFace(Ptr<DgramChannel> channel)
  : DgramFace(channel, NetworkAddress()) {
  this->set_kind(FaceKind::kMulticast);
}

DgramChannel::DgramChannel(int fd, const NetworkAddress& local_addr, Ptr<AddressVerifier> av, Ptr<WireProtocol> wp) {
  assert(fd >= 0);
  assert(av != nullptr);
  assert(wp != nullptr);
  this->fd_ = fd;
  this->local_addr_ = local_addr;
  this->av_ = av;
  this->wp_ = wp;
  this->recvbuf_ = new Buffer(0);
}

void DgramChannel::Init(void) {
  this->fallback_face_ = this->New<DgramFallbackFace>(this);
  this->fallback_face_->set_kind(FaceKind::kMulticast);
  this->global()->pollmgr()->Add(this, this->fd(), POLLIN);
}

DgramChannel::~DgramChannel(void) {
  this->Close();
}

Ptr<DgramFace> DgramChannel::CreateFace(const NetworkAddress& peer) {
  Ptr<DgramFace> face = this->New<DgramFace>(this, peer);
  bool is_local = this->av()->IsLocal(peer) || this->av()->AreSameHost(this->local_addr(), peer);
  if (is_local) {
    face->set_kind(FaceKind::kApp);
  } else {
    face->set_kind(FaceKind::kUnicast);
  }
  return face;
}

DgramChannel::PeerEntry DgramChannel::GetOrCreatePeer(const NetworkAddress& peer, bool create_face) {
  Ptr<DgramFace> face; Ptr<WireProtocolState> wps;
  auto it = this->peers_.find(peer);
  if (it != this->peers_.end()) {
    std::tie(face, wps) = it->second;
    if (create_face && face == nullptr) {
      face = this->CreateFace(peer);
      return this->peers_[peer] = std::make_tuple(face, wps);
    }
    return it->second;
  }

  face = create_face ? this->New<DgramFace>(this, peer) : nullptr;
  wps = this->wp()->IsStateful() ? this->wp()->CreateState(peer) : nullptr;
  PeerEntry entry = std::make_tuple(face, wps);
  this->peers_.insert(std::make_pair(peer, entry));
  return entry;
}

Ptr<DgramFace> DgramChannel::GetFace(const NetworkAddress& peer) {
  Ptr<DgramFace> face; Ptr<WireProtocolState> wps;
  std::tie(face, wps) = this->GetOrCreatePeer(peer, true);
  return face;
}

void DgramChannel::FaceSend(Ptr<DgramFace> face, Ptr<Message> message) {
  Ptr<DgramFace> oface; Ptr<WireProtocolState> wps;
  std::tie(oface, wps) = this->GetOrCreatePeer(face->peer(), false);
  assert(oface == face);
  
  bool ok; std::list<Ptr<Buffer>> pkts;
  std::tie(ok, pkts) = this->wp()->Encode(face->peer(), wps, message);
  if (!ok) {
    this->Log(kLLWarn, kLCFace, "DgramChannel::FaceSend(%"PRI_FaceId") protocol error", face->id());
    return;
  }
  for (Ptr<Buffer> pkt : pkts) {
    this->SendTo(face->peer(), pkt);
  }
}

void DgramChannel::SendTo(const NetworkAddress& peer, Ptr<Buffer> pkt) {
  sendto(this->fd(), pkt->data(), pkt->length(), 0, reinterpret_cast<const sockaddr*>(&peer.who), peer.wholen);
}

void DgramChannel::PollCallback(int fd, short revents) {
  assert(fd == this->fd());
  if ((revents & POLLIN) != 0) {
    this->ReceiveFrom();
  }
}

void DgramChannel::ReceiveFrom(void) {
  Ptr<Buffer> pkt = this->recvbuf(); pkt->Reset(); size_t buflen = 65536;
  NetworkAddress peer;
  ssize_t res = recvfrom(this->fd(), pkt->Reserve(buflen), buflen, 0, reinterpret_cast<sockaddr*>(&peer.who), &peer.wholen);
  if (res <= 0) return;
  pkt->Put(static_cast<size_t>(res));
  this->DeliverPacket(peer, pkt);
}

void DgramChannel::DeliverPacket(const NetworkAddress& peer, Ptr<Buffer> pkt) {
  Ptr<DgramFace> face; Ptr<WireProtocolState> wps;
  std::tie(face, wps) = this->GetOrCreatePeer(peer, false);
  
  bool ok; std::list<Ptr<Message>> msgs;
  std::tie(ok, msgs) = this->wp()->Decode(peer, wps, pkt);
  if (!ok) {
    this->Log(kLLWarn, kLCFace, "DgramChannel::DeliverPacket(%"PRI_FaceId") protocol error", face->id());
    return;
  }

  if (face == nullptr) face = this->GetFallbackFace();
  for (Ptr<Message> msg : msgs) {
    face->Deliver(msg);
  }
}

void DgramChannel::Close() {
  for (auto p : this->peers()) {
    Ptr<DgramFace> face = std::get<0>(p.second);
    if (face != nullptr) {
      face->Close();
    }
  }
  this->peers().clear();
  this->GetFallbackFace()->Close();
  close(this->fd());
  this->global()->pollmgr()->RemoveAll(this);
}


};//namespace ndnfd
