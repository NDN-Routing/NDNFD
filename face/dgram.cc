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
  this->channel()->FaceClose(this);
  this->CloseInternal();
}

void DgramFace::CloseInternal(void) {
  this->set_status(FaceStatus::kClosed);
  this->channel_ = nullptr;
}

DgramFallbackFace::DgramFallbackFace(Ptr<DgramChannel> channel)
  : DgramFace(channel, NetworkAddress()) {
  this->set_kind(FaceKind::kMulticast);
}

void DgramFallbackFace::Close(void) {
  this->channel()->Close();
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
  this->closed_ = false;
}

void DgramChannel::Init(void) {
  this->fallback_face_ = this->New<DgramFallbackFace>(this);
  this->fallback_face_->set_kind(FaceKind::kMulticast);
  this->global()->pollmgr()->Add(this, this->fd(), POLLIN);
  this->Log(kLLInfo, kLCFace, "DgramChannel(%"PRIxPTR",fd=%d)::Init local=%s fallback=%"PRI_FaceId"", this, this->fd(), this->av()->ToString(this->local_addr_).c_str(), this->fallback_face_->id());
}

DgramChannel::~DgramChannel(void) {
  this->Close();
}

Ptr<DgramFace> DgramChannel::CreateFace(const AddressHashKey& hashkey, const NetworkAddress& peer) {
  Ptr<DgramFace> face = this->New<DgramFace>(this, peer);
  bool is_local = this->av()->IsLocal(peer) || this->av()->AreSameHost(this->local_addr(), peer);
  if (is_local) {
    face->set_kind(FaceKind::kApp);
  } else {
    face->set_kind(FaceKind::kUnicast);
  }
  this->Log(kLLInfo, kLCFace, "DgramChannel(%"PRIxPTR",fd=%d)::CreateFace id=%"PRI_FaceId" peer=%s", this, this->fd(), face->id(), this->av()->ToString(peer).c_str());
  return face;
}

DgramChannel::PeerEntry DgramChannel::MakePeer(const NetworkAddress& peer, MakePeerFaceOp face_op) {
  AddressHashKey hashkey = this->av()->GetHashKey(peer);
  Ptr<DgramFace> face; Ptr<WireProtocolState> wps;
  auto it = this->peers_.find(hashkey);
  if (it != this->peers_.end()) {
    std::tie(face, wps) = it->second;
    if (face_op == MakePeerFaceOp::kCreate && face == nullptr) {
      face = this->CreateFace(hashkey, peer);
      return this->peers_[hashkey] = std::make_tuple(face, wps);
    } else if (face_op == MakePeerFaceOp::kDelete && face != nullptr) {
      face = nullptr;
      return this->peers_[hashkey] = std::make_tuple(face, wps);
    }
    return it->second;
  }

  face = face_op == MakePeerFaceOp::kCreate ? this->CreateFace(hashkey, peer) : nullptr;
  wps = this->wp()->IsStateful() ? this->wp()->CreateState(peer) : nullptr;
  PeerEntry entry = std::make_tuple(face, wps);
  this->peers_.insert(std::make_pair(hashkey, entry));
  return entry;
}

Ptr<DgramFace> DgramChannel::GetFace(const NetworkAddress& peer) {
  Ptr<DgramFace> face; Ptr<WireProtocolState> wps;
  std::tie(face, wps) = this->MakePeer(peer, MakePeerFaceOp::kCreate);
  return face;
}

void DgramChannel::FaceSend(Ptr<DgramFace> face, Ptr<Message> message) {
  Ptr<DgramFace> oface; Ptr<WireProtocolState> wps;
  std::tie(oface, wps) = this->MakePeer(face->peer(), MakePeerFaceOp::kNone);
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
  std::tie(face, wps) = this->MakePeer(peer, MakePeerFaceOp::kNone);
  //this->Log(kLLDebug, kLCFace, "DgramChannel(%"PRIxPTR",fd=%d)::DeliverPacket(%s) face=%"PRI_FaceId"", this, this->fd(), this->av()->ToString(peer).c_str(), face==nullptr ? FaceId_none : face->id());
  
  bool ok; std::list<Ptr<Message>> msgs;
  std::tie(ok, msgs) = this->wp()->Decode(peer, wps, pkt);
  if (!ok) {
    this->Log(kLLWarn, kLCFace, "DgramChannel(%"PRIxPTR",fd=%d)::DeliverPacket(%s) protocol error", this, this->fd(), this->av()->ToString(peer).c_str());
    return;
  }

  if (face == nullptr) face = this->GetFallbackFace();
  for (Ptr<Message> msg : msgs) {
    face->Deliver(msg);
  }
}

void DgramChannel::FaceClose(Ptr<DgramFace> face) {
  this->MakePeer(face->peer(), MakePeerFaceOp::kDelete);
}

void DgramChannel::Close() {
  if (this->closed_) return;
  this->closed_ = true;
  
  for (auto p : this->peers()) {
    Ptr<DgramFace> face = std::get<0>(p.second);
    if (face != nullptr) {
      face->CloseInternal();
    }
  }
  this->peers().clear();
  this->GetFallbackFace()->CloseInternal();
  close(this->fd());
  this->global()->pollmgr()->RemoveAll(this);
}


};//namespace ndnfd
