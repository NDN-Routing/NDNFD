#include "dgram.h"
#include <unistd.h>
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

void DgramFace::Send(Ptr<const Message> message) {
  if (!this->CanSend()) {
    this->Log(kLLError, kLCFace, "DgramFace(%" PRI_FaceId ")::Send !CanSend", this->id());
    return;
  }
  this->channel()->FaceSend(this, message);
}

bool DgramFace::SendReachable(Ptr<const Face> other) const {
  assert(other != nullptr);
  
  if (this->kind() == FaceKind::kMulticast && this->CanSend() && DgramFace::IsDgramFaceType(other->type())) {
    const DgramFace* other_face = static_cast<const DgramFace*>(PeekPointer(other));
    // Assume everyone on a channel joins the multicast group.
    // This can be mandated for Ethernet.
    // UDP mcast is unreliable, but it's on a separate port number (and separate channel).
    return this->channel() == other_face->channel();
  }
  return false;
}

void DgramFace::Deliver(Ptr<Message> msg) {
  if (this->status() == FaceStatus::kUndecided) this->set_status(FaceStatus::kEstablished);
  this->ReceiveMessage(msg);
}

void DgramFace::DoFinalize(void) {
  this->channel()->FaceClose(this);
  this->channel_ = nullptr;
}

DgramFallbackFace::DgramFallbackFace(Ptr<DgramChannel> channel)
  : DgramFace(channel, NetworkAddress()) {
  this->set_kind(FaceKind::kMulticast);
}

void DgramFallbackFace::DoFinalize(void) {
  this->channel()->Close();
}

DgramChannel::DgramChannel(int fd, const NetworkAddress& local_addr, Ptr<const AddressVerifier> av, Ptr<const WireProtocol> wp) {
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
  if (this->fd() >= 0) this->global()->pollmgr()->Add(this, this->fd(), POLLIN);
  this->global()->scheduler()->Schedule(DgramChannel::kReapInterval, std::bind(&DgramChannel::ReapInactivePeers, this), &this->reap_evt_);

  this->Log(kLLInfo, kLCFace, "DgramChannel(%" PRIxPTR ",fd=%d)::Init local=%s fallback=%" PRI_FaceId "", this, this->fd(), this->av()->ToString(this->local_addr_).c_str(), this->fallback_face_->id());
}

Ptr<DgramFace> DgramChannel::CreateFace(const AddressHashKey& hashkey, const NetworkAddress& peer) {
  Ptr<DgramFace> face = this->New<DgramFace>(this, peer);
  bool is_local = this->av()->IsLocal(peer) || this->av()->AreSameHost(this->local_addr(), peer);
  if (is_local) {
    face->set_kind(FaceKind::kApp);
  } else {
    face->set_kind(FaceKind::kUnicast);
  }
  this->Log(kLLInfo, kLCFace, "DgramChannel(%" PRIxPTR ",fd=%d)::CreateFace id=%" PRI_FaceId " peer=%s", this, this->fd(), face->id(), this->av()->ToString(peer).c_str());
  return face;
}

Ptr<DgramChannel::PeerEntry> DgramChannel::MakePeer(const NetworkAddress& peer, MakePeerFaceOp face_op) {
  assert(this->av()->Check(peer));
  
  AddressHashKey hashkey = this->av()->GetHashKey(peer);
  Ptr<PeerEntry> pe;
  auto it = this->peers_.find(hashkey);
  if (it != this->peers_.end()) {
    pe = it->second;
    if (face_op == MakePeerFaceOp::kCreate && pe->face_ == nullptr) {
      pe->face_ = this->CreateFace(hashkey, peer);
    } else if (face_op == MakePeerFaceOp::kDelete && pe->face_ != nullptr) {
      pe->face_ = nullptr;
    }
    return pe;
  }

  pe = new PeerEntry();
  if (face_op == MakePeerFaceOp::kCreate) pe->face_ = this->CreateFace(hashkey, peer);
  if (this->wp()->IsStateful()) pe->wps_ = this->wp()->CreateState(peer);
  this->peers_.insert(std::make_pair(hashkey, pe));
  return pe;
}

constexpr std::chrono::microseconds DgramChannel::kReapInterval;

std::chrono::microseconds DgramChannel::ReapInactivePeers(void) {
  int closed = 0;
  
  for (auto it = this->peers().begin(); it != this->peers().end();) {
    auto current = it++;
    Ptr<PeerEntry> pe = current->second;
    if (pe->recv_count_ == 0) {
      if (++closed == 1) {
        this->Log(kLLDebug, kLCFace, "DgramChannel(%" PRIxPTR ")::ReapInactivePeers", this);
      }
      if (pe->face_ != nullptr) pe->face_->Close();
      this->peers().erase(current);
    }
    pe->recv_count_ = 0;
  }
  
  return DgramChannel::kReapInterval;
}

void DgramChannel::FaceSend(Ptr<DgramFace> face, Ptr<const Message> message) {
  Ptr<DgramFace> oface; Ptr<WireProtocolState> wps;
  if (face->kind() == FaceKind::kMulticast) {
    Ptr<McastEntry> entry = this->FindMcastEntry(face->peer());
    assert(entry != nullptr);
    oface = entry->face_;
    wps = entry->outstate_;
  } else {
    Ptr<PeerEntry> pe = this->MakePeer(face->peer(), MakePeerFaceOp::kNone);
    oface = pe->face_;
    wps = pe->wps_;
  }
  assert(oface == face);
  
  bool ok; std::list<Ptr<Buffer>> pkts;
  std::tie(ok, pkts) = this->wp()->Encode(face->peer(), wps, message);
  if (!ok) {
    this->Log(kLLWarn, kLCFace, "DgramChannel::FaceSend(%" PRI_FaceId ") protocol error", face->id());
    return;
  }
  //this->Log(kLLDebug, kLCFace, "DgramChannel::FaceSend(%"  PRI_FaceId") send %" PRIuMAX " packets", face->id(), static_cast<uintmax_t>(pkts.size()));
  for (Ptr<Buffer> pkt : pkts) {
    face->CountBytesOut(pkt->length());
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

void DgramChannel::DeliverPacket(const NetworkAddress& peer, Ptr<BufferView> pkt) {
  Ptr<PeerEntry> pe = this->MakePeer(peer, MakePeerFaceOp::kNone);
  ++pe->recv_count_;
  Ptr<DgramFace> face = pe->face_;
  //this->Log(kLLDebug, kLCFace, "DgramChannel(%"PRIxPTR",fd=%d)::DeliverPacket(%s) face=%"PRI_FaceId"", this, this->fd(), this->av()->ToString(peer).c_str(), face==nullptr ? FaceId_none : face->id());
  
  if (face == nullptr) face = this->GetFallbackFace();
  this->DecodeAndDeliver(peer, pe->wps_, pkt, face);
}

void DgramChannel::DecodeAndDeliver(const NetworkAddress& peer, Ptr<WireProtocolState> wps, Ptr<BufferView> pkt, Ptr<DgramFace> face) {
  assert(pkt != nullptr);
  assert(face != nullptr);
  face->CountBytesIn(pkt->length());

  bool ok; std::list<Ptr<Message>> msgs;
  std::tie(ok, msgs) = this->wp()->Decode(peer, wps, pkt);
  if (!ok) {
    this->Log(kLLWarn, kLCFace, "DgramChannel(%" PRIxPTR ",fd=%d)::DecodeAndDeliver(%s) protocol error", this, this->fd(), this->av()->ToString(peer).c_str());
    return;
  }

  //this->Log(kLLDebug, kLCFace, "DgramChannel(%" PRIxPTR ",fd=%d)::DecodeAndDeliver(%s) deliver %" PRIuMAX " messages to face %" PRI_FaceId, this, this->fd(), this->av()->ToString(peer).c_str(), static_cast<uintmax_t>(msgs.size()), face->id());
  for (Ptr<Message> msg : msgs) {
    msg->set_incoming_sender(peer);
    face->Deliver(msg);
  }
}

Ptr<DgramFace> DgramChannel::GetMcastFace(const NetworkAddress& group) {
  Ptr<McastEntry> entry = this->MakeMcastEntry(group);
  if (entry == nullptr) return nullptr;
  return entry->face_;
}

Ptr<DgramChannel::McastEntry> DgramChannel::FindMcastEntryInternal(const NetworkAddress& group, bool create) {
  AddressHashKey hashkey = this->av()->GetHashKey(group);
  for (Ptr<McastEntry> entry : this->mcasts()) {
    if (entry->group_ == hashkey) {
      return entry;
    }
  }
  if (!create) return nullptr;
  Ptr<DgramFace> face = this->CreateMcastFace(hashkey, group);
  if (face == nullptr) return nullptr;
  this->Log(kLLInfo, kLCFace, "DgramChannel(%" PRIxPTR ",fd=%d)::CreateMcastFace id=%" PRI_FaceId " group=%s", this, this->fd(), face->id(), this->av()->ToString(group).c_str());

  Ptr<McastEntry> entry = new McastEntry(hashkey);
  entry->face_ = face;
  if (this->wp()->IsStateful()) entry->outstate_ = this->wp()->CreateState(group);
  this->mcasts().push_back(entry);
  return entry;
}

void DgramChannel::DeliverMcastPacket(const NetworkAddress& group, const NetworkAddress& peer, Ptr<BufferView> pkt) {
  if (group == this->local_addr()) {
    this->DeliverPacket(peer, pkt);
    return;
  }
  Ptr<McastEntry> entry = this->FindMcastEntry(group);
  this->DeliverMcastPacket(entry, peer, pkt);
}

void DgramChannel::DeliverMcastPacket(Ptr<McastEntry> entry, const NetworkAddress& peer, Ptr<BufferView> pkt) {
  if (entry == nullptr) return;//mcast face must be explicitly created

  //this->Log(kLLDebug, kLCFace, "DgramChannel(%" PRIxPTR ",fd=%d)::DeliverMcastPacket(%s,%s) face=%" PRI_FaceId "", this, this->fd(), this->av()->ToString(peer).c_str(), this->av()->ToString(entry->face_->peer()).c_str(), entry->face_->id());

  Ptr<WireProtocolState> wps = nullptr;
  if (this->wp()->IsStateful()) {
    AddressHashKey hashkey = this->av()->GetHashKey(peer);
    auto it = entry->instate_.find(hashkey);
    if (it == entry->instate_.end()) {
      wps = this->wp()->CreateState(peer);
      entry->instate_[hashkey] = wps;
    } else {
      wps = it->second;
    }
  }
  
  this->DecodeAndDeliver(peer, wps, pkt, entry->face_);
}

void DgramChannel::FaceClose(Ptr<DgramFace> face) {
  this->MakePeer(face->peer(), MakePeerFaceOp::kDelete);
}

void DgramChannel::CloseFd() {
  if (this->fd() >= 0) close(this->fd());
}

void DgramChannel::Close() {
  if (this->closed_) return;
  this->closed_ = true;
  
  for (auto p : this->peers()) {
    Ptr<DgramFace> face = p.second->face_;
    face->Close();
  }
  this->peers().clear();
  this->GetFallbackFace()->Close();
  this->global()->pollmgr()->RemoveAll(this);
  this->CloseFd();
  this->global()->scheduler()->Cancel(this->reap_evt_);
}


};//namespace ndnfd
