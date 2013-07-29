#include "asl.h"
#include "face/facemgr.h"
namespace ndnfd {

StrategyType_def(AslStrategy, adaptive_selflearn);

std::unordered_set<FaceId> AslStrategy::LookupOutbounds(Ptr<PitEntry> ie, Ptr<const InterestMessage> interest) {
  // upstream lookup is integrated in AslStrategy::Forward for better performance
  return std::unordered_set<FaceId>();
}

void AslStrategy::Forward(Ptr<PitEntry> ie) {
  struct UpstreamScore {
    FaceId face_;
    bool new_producer_;
    UpstreamStatus status_;
    bool unexpired_unicast_downstream_;
    std::chrono::microseconds rto_;
  };
  UpstreamScore best;// current best upstream
  best.face_ = FaceId_none;
  // ReplaceUpstreamIfBetter overwrites best if alt is better than best
  std::function<void(const UpstreamScore&)> ReplaceUpstreamIfBetter = [&best] (const UpstreamScore& alt) {
    if (best.face_ == FaceId_none) {
      best = alt; return;
    }
    if (alt.new_producer_) {
      if (alt.new_producer_ && !best.new_producer_) best = alt;
      return;
    }
    if (alt.status_ != best.status_) {
      if (alt.status_ > best.status_) best = alt;
      return;
    }
    if (alt.unexpired_unicast_downstream_ != best.unexpired_unicast_downstream_) {
      if (!alt.unexpired_unicast_downstream_ && best.unexpired_unicast_downstream_) best = alt;
      return;
    }
    if (alt.rto_ < best.rto_) best = alt;
  };
  
  // prepare shared information
  int downstream_count = 0;
  FaceId sole_downstream = FaceId_none;
  std::vector<Ptr<Face>> mcast_downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    ++downstream_count;
    sole_downstream = p->faceid();
    Ptr<Face> f = this->global()->facemgr()->GetFace(p->faceid());
    if (f != nullptr && f->kind() == FaceKind::kMulticast) mcast_downstreams.push_back(f);
  });
  if (downstream_count > 1) sole_downstream = FaceId_none;
  
  // Reject1 evaluates certain rules that reject an upstream
  std::function<std::tuple<bool,Ptr<Face>>(FaceId)> Reject1 = [&](FaceId face) ->std::tuple<bool,Ptr<Face>> {
    if (face == sole_downstream) return std::forward_as_tuple(true, nullptr);
    Ptr<PitUpstreamRecord> p = ie->GetUpstream(face);
    // PFI_VAIN cannot be written as NacksStrategy::PFI_VAIN due to gcc bug http://stackoverflow.com/a/12086873
    if (p != nullptr && p->GetFlag(PFI_VAIN)) return std::forward_as_tuple(true, nullptr);
    
    Ptr<Face> f = this->global()->facemgr()->GetFace(face);
    if (f == nullptr) return std::forward_as_tuple(true, nullptr);
    for (Ptr<Face> mcast_face : mcast_downstreams) {
      if (mcast_face->SendReachable(f)) {
        return std::forward_as_tuple(true, nullptr);
      }
    }
    return std::forward_as_tuple(false, f);
  };
  
  // score each known upstream
  std::chrono::microseconds min_rto(500000);
  NpeExtra* extra = ie->npe()->GetStrategyExtra<NpeExtra>();
  for (auto pair : extra->table_) {
    std::chrono::microseconds rto = pair.second.rtt_.RetransmitTimeout();
    min_rto = std::min(min_rto, rto);
    
    bool reject; Ptr<Face> f;
    std::tie(reject, f) = Reject1(pair.first);
    if (reject) continue;
    if (pair.second.status_ == UpstreamStatus::kRed) continue;

    UpstreamScore alt;
    alt.face_ = pair.first;
    alt.new_producer_ = false;
    alt.status_ = pair.second.status_;
    alt.unexpired_unicast_downstream_ = false;
    if (f->kind() == FaceKind::kUnicast) {
      Ptr<PitDownstreamRecord> downstream = ie->GetDownstream(pair.first);
      if (downstream != nullptr && !downstream->IsExpired()) {
        alt.unexpired_unicast_downstream_ = true;
      }
    }
    alt.rto_ = rto;
    ReplaceUpstreamIfBetter(alt);
  }
  
  // score each unknown upstream
  auto pit_result = ie->npe()->LookupFib(ie->interest());
  for (FaceId face : pit_result) {
    if (extra->table_.count(face) != 0) continue;
    bool reject; Ptr<Face> f;
    std::tie(reject, f) = Reject1(face);
    if (reject) continue;
    
    UpstreamScore alt;
    alt.face_ = face;
    alt.new_producer_ = true;
    ReplaceUpstreamIfBetter(alt);
  }
  
  if (best.face_ == FaceId_none) {// no feasible upstream
    this->Flood(ie);
    return;
  }
  
  std::chrono::microseconds predict;
  if (best.new_producer_) {
    predict = min_rto * 2;
  } else {
    predict = best.rto_;
  }
  
  Ptr<PitUpstreamRecord> p = ie->SeekUpstream(best.face_);
  this->SendInterest(ie, p);
  ie->RttStartWithExpect(p->faceid(), predict, std::bind(&AslStrategy::DidnotArriveOnFace, this, ie, p->faceid()));
  
  this->Log(kLLDebug, kLCStrategy, "AslStrategy::Forward(%" PRI_PitEntrySerial ") upstream=%" PRI_FaceId " predict=%" PRIuMAX "", ie->serial(), p->faceid(), static_cast<uintmax_t>(predict.count()));
}

void AslStrategy::Flood(Ptr<PitEntry> ie) {
  std::unordered_set<FaceId> unexpired_downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    if (!p->IsExpired()) unexpired_downstreams.insert(p->faceid());
  });
  
  std::vector<Ptr<Face>> mcast_faces; std::vector<Ptr<Face>> unicast_downstream_faces;
  for (Ptr<Face> f : *this->global()->facemgr()) {
    if (f->kind() == FaceKind::kMulticast && f->CanSend() && unexpired_downstreams.count(f->id()) == 0) {
      mcast_faces.push_back(f);
    }
    if (f->kind() == FaceKind::kUnicast && unexpired_downstreams.count(f->id()) > 0) {
      unicast_downstream_faces.push_back(f);
    }
  }
  
  int flood_count = 0;
  std::string debug_list; char debug_buf[32];
#define DEBUG_APPEND_FaceId(x) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ",", x); debug_list.append(debug_buf); }
  for (Ptr<Face> f : mcast_faces) {
    if (unicast_downstream_faces.size() == 1 && f->SendReachable(unicast_downstream_faces[0])) {
      continue;
    }
    Ptr<PitUpstreamRecord> p = ie->SeekUpstream(f->id());
    p->SetFlag(AslStrategy::PFI_FLOOD, true);
    this->SendInterest(ie, p);// TODO avoid using a unicast downstream reachable on p
    ie->RttStart(p->faceid());
    ++flood_count;
    DEBUG_APPEND_FaceId(p->faceid());
  }
  
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
#undef DEBUG_APPEND_FaceId
  this->Log(kLLDebug, kLCStrategy, "AslStrategy::Flood(%" PRI_PitEntrySerial ") upstreams=[%s]", ie->serial(), debug_list.c_str());

  if (flood_count == 0) {
    this->SendNacks(ie, NackCode::kNoData);
  }

  std::chrono::microseconds min_rto(500000);
  NpeExtra* extra = ie->npe()->GetStrategyExtra<NpeExtra>();
  for (auto pair : extra->table_) {
    std::chrono::microseconds rto = pair.second.rtt_.RetransmitTimeout();
    min_rto = std::min(min_rto, rto);
  }
  this->SetRetryTimer(ie, 2 * min_rto);
}

void AslStrategy::OnRetryTimerExpire(Ptr<PitEntry> ie) {
  if (std::any_of(ie->beginUpstream(), ie->endUpstream(), [] (Ptr<PitUpstreamRecord> p) { return !p->IsExpired() && p->GetFlag(AslStrategy::PFI_FLOOD); })) {
    this->SendNacks(ie, NackCode::kNoData);
  }
}

void AslStrategy::DidSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int pending_downstreams) {
  this->CancelRetryTimer(ie);
  std::chrono::microseconds rtt = ie->RttEnd(co->incoming_face());
  
  Ptr<Face> src = this->global()->facemgr()->GetFace(co->incoming_face());
  if (src == nullptr) return;
  Ptr<Face> peer = src;
  if (src->kind() == FaceKind::kMulticast) {
    peer = this->global()->facemgr()->MakeUnicastFace(src, co->incoming_sender());
  }

  this->Log(kLLDebug, kLCStrategy, "AslStrategy::DidSatisfyPendingInterest(%" PRI_PitEntrySerial ") src=%" PRI_FaceId " peer=%" PRI_FaceId " rtt=%" PRIuMAX "", ie->serial(), src->id(), peer->id(), static_cast<uintmax_t>(rtt.count()));
  
  this->ForeachNpeAncestor(ie->npe(), [&] (Ptr<NamePrefixEntry> npe) {
    NpeExtra* extra = npe->GetStrategyExtra<NpeExtra>();
    UpstreamExtra& ue = extra->table_[peer->id()];
    ue.status_ = UpstreamStatus::kGreen;
    if (rtt.count() >= 0) ue.rtt_.Measurement(rtt);
  });
}

void AslStrategy::SendNack(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, NackCode code) {
  Ptr<Face> dst = this->global()->facemgr()->GetFace(downstream->faceid());
  if (dst == nullptr || dst->kind() == FaceKind::kMulticast) return;
  this->StrategyBase::SendNack(ie, downstream, code);
}

void AslStrategy::ProcessNack(Ptr<PitEntry> ie, Ptr<const NackMessage> nack) {
  Ptr<Face> src = this->global()->facemgr()->GetFace(nack->incoming_face());
  if (src == nullptr || src->kind() == FaceKind::kMulticast) return;
  ie->RttEnd(nack->incoming_face());// cancel DidnotArriveOnFace timer

  this->ForeachNpeAncestor(ie->npe(), [&] (Ptr<NamePrefixEntry> npe) {
    NpeExtra* extra = npe->GetStrategyExtra<NpeExtra>();
    UpstreamExtra& ue = extra->table_[src->id()];
    if (ue.status_ == UpstreamStatus::kGreen) ue.status_ = UpstreamStatus::kYellow;
    ue.rtt_.IncrementMultiplier();
  });

  Ptr<PitUpstreamRecord> upstream = ie->GetUpstream(nack->incoming_face());
  if (upstream != nullptr) upstream->SetFlag(NacksStrategy::PFI_VAIN, true);

  this->Log(kLLDebug, kLCStrategy, "AslStrategy::ProcessNack(%" PRI_PitEntrySerial ") src=%" PRI_FaceId "", ie->serial(), src->id());

  this->Forward(ie);
}

void AslStrategy::DidnotArriveOnFace(Ptr<PitEntry> ie, FaceId face) {
  this->ForeachNpeAncestor(ie->npe(), [&] (Ptr<NamePrefixEntry> npe) {
    NpeExtra* extra = npe->GetStrategyExtra<NpeExtra>();
    UpstreamExtra& ue = extra->table_[face];
    if (ue.status_ == UpstreamStatus::kGreen) ue.status_ = UpstreamStatus::kYellow;
    ue.rtt_.IncreaseMultiplier();
  });

  Ptr<PitUpstreamRecord> upstream = ie->GetUpstream(face);
  if (upstream != nullptr) upstream->SetFlag(NacksStrategy::PFI_VAIN, true);
}

void AslStrategy::InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent) {
  NpeExtra* parent_extra = parent->GetStrategyExtra<NpeExtra>();
  npe->set_strategy_extra(new NpeExtra(*parent_extra));
}

void AslStrategy::ForeachNpeAncestor(Ptr<NamePrefixEntry> npe, std::function<void(Ptr<NamePrefixEntry>)> f) {
  Ptr<NamePrefixEntry> end1 = npe->StrategyNode();
  Ptr<NamePrefixEntry> end2 = npe->FibNode();
  
  for (Ptr<NamePrefixEntry> n = npe; n != nullptr; n = n->Parent()) {
    f(n);
    if (n == end1 || n == end2) break;
  }
}

};//namespace ndnfd
