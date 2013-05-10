#include "selflearn.h"
#include <algorithm>
#include "face/facemgr.h"
#include "core/scheduler.h"
extern "C" {
uint32_t WTHZ_value(void);
}
namespace ndnfd {

SelfLearnStrategy::NpeExtra* SelfLearnStrategy::GetExtra(Ptr<NamePrefixEntry> npe) {
  NpeExtra* extra = npe->strategy_extra<NpeExtra>();
  if (extra != nullptr) return extra;

  Ptr<NamePrefixEntry> parent = npe->Parent();
  if (parent == nullptr) {
    extra = new NpeExtra();
  } else {
    extra = new NpeExtra();
    extra->predicts_ = this->GetExtra(parent)->predicts_;
  }
  npe->set_strategy_extra(extra);
  return extra;
}

void SelfLearnStrategy::RankPredicts(NpeExtra* extra) {
  assert(extra != nullptr);
  std::multimap<std::chrono::microseconds,FaceId> m;
  //for (auto pair : extra->predicts_) {
  //  pair.second.time_ = std::chrono::duration_cast<std::chrono::microseconds>(pair.second.rtt_.RetransmitTimeout());
  //  m.insert(std::make_pair(pair.second.time_, pair.first));
  //}
  for (auto it = extra->predicts_.begin(); it != extra->predicts_.end(); ++it) {
    it->second.time_ = std::chrono::duration_cast<std::chrono::microseconds>(it->second.rtt_.RetransmitTimeout());
    m.insert(std::make_pair(it->second.time_, it->first));
  }
  int rank = 0; std::chrono::microseconds accum(0);
  for (auto pair : m) {
    PredictRecord& pr = extra->predicts_[pair.second];
    pr.rank_ = rank++;
    pr.accum_ = (accum += pr.time_);
  }
}

std::unordered_set<FaceId> SelfLearnStrategy::LookupOutbounds(Ptr<PitEntry> ie, Ptr<InterestMessage> interest) {
  Ptr<NamePrefixEntry> npe = ie->npe();
  
  // collect downstreams
  std::vector<Ptr<Face>> downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    Ptr<Face> downstream = this->global()->facemgr()->GetFace(p->faceid());
    if (downstream != nullptr) downstreams.push_back(downstream);
  });

  // lookup FIB
  std::unordered_set<FaceId> fib_candidates = npe->LookupFib(interest);
  std::unordered_set<FaceId> candidates = fib_candidates;
  // also include (inherited) best face
  //candidates.insert(npe->GetBestFace());
  //candidates.insert(npe->prev_faceid());
  //candidates.erase(FaceId_none);
  for (auto tuple : this->GetExtra(npe)->predicts_) {
    candidates.insert(std::get<0>(tuple));
  }
  
  // select outbounds
  std::unordered_set<FaceId> outbounds;
  for (FaceId outbound : candidates) {
    Ptr<Face> upstream = this->global()->facemgr()->GetFace(outbound);
    if (upstream == nullptr || !upstream->CanSend()) continue;
    switch (upstream->kind()) {
      case FaceKind::kInternal:
      case FaceKind::kApp:// exclude local faces not in FIB: they always register prefix
        if (fib_candidates.find(outbound) == fib_candidates.end()) continue;
      default:
        break;
    }
    bool reach = false;
    for (Ptr<Face> downstream : downstreams) {
      // if outbound unicast is a downstream: it's a consumer
      // if outbound mcast is a downstream: potential producer has seen the Interest
      // if outbound unicast is reachable on a downstream mcast face: potential producer has seen the Interest
      // if outbound mcast can reach a downstream unicast face: downstream will broadcast
      if (downstream->id() == outbound || downstream->SendReachable(upstream) || upstream->SendReachable(downstream)) {
        reach = true;
        break;
      }
    }
    if (!reach) {
      outbounds.insert(outbound);
    }
  }
  
  return outbounds;
}

void SelfLearnStrategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  // find downstream
  auto first_downstream = ie->beginDownstream();
  assert(first_downstream != ie->endDownstream());
  Ptr<PitDownstreamRecord> downstream = *first_downstream;
  assert(downstream != nullptr && downstream->pending());
  
  // flood if no outbound
  if (ie->beginUpstream() == ie->endUpstream()) {
    this->StartFlood(ie);
    return;
  }

  std::chrono::microseconds lifetime = downstream->time_until_expiry();
  std::chrono::microseconds flood_time = lifetime / 2;
  
  // assign expiry time for outbounds
  Ptr<NamePrefixEntry> npe = ie->npe();
  NpeExtra* extra = this->GetExtra(npe);
  this->RankPredicts(extra);
  
  std::string debug_list;
  char debug_buf[32];
#define DEBUG_APPEND_FaceTime(face,s,time) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ":%s%" PRIu32 ",", face,s,static_cast<uint32_t>(time)); debug_list.append(debug_buf); }

  std::chrono::microseconds flood_actual(0);
  for (auto it = ie->beginUpstream(); it != ie->endUpstream(); ++it) {
    Ptr<PitUpstreamRecord> upstream = *it;
    FaceId faceid = upstream->faceid();
    if (faceid == downstream->faceid()) continue;
    auto it2 = extra->predicts_.find(faceid);
    if (it2 == extra->predicts_.end()) {
      // no prediction: use it now
      DEBUG_APPEND_FaceTime(faceid,"new",0);
      upstream->SetExpiry(std::chrono::microseconds::zero());
      PredictRecord& pr1 = extra->predicts_[faceid];
      pr1.time_ = SelfLearnStrategy::initial_prediction();// used for logging timeout
      flood_actual = std::max(pr1.time_, flood_actual);
      continue;
    }
    PredictRecord& pr = it2->second;
    if (pr.rank_ == 0) {
      // best face: use it now
      DEBUG_APPEND_FaceTime(faceid,"best",pr.time_.count());
      upstream->SetExpiry(std::chrono::microseconds::zero());
      flood_actual = std::max(pr.time_, flood_actual);
    } else if (pr.accum_ < flood_time) {
      // non-best face: round-robin after best face timeout
      DEBUG_APPEND_FaceTime(faceid,"",pr.time_.count());
      upstream->SetExpiry(pr.accum_ - pr.time_);
      flood_actual = std::max(pr.time_, flood_actual);
    } else {
      // worst face: don't use them
      DEBUG_APPEND_FaceTime(faceid,"no",pr.time_.count());
      upstream->SetExpiry(lifetime);
    }
  }
  
  flood_actual = std::min(flood_actual, flood_time);
#undef DEBUG_APPEND_FaceTime
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::PropagateNewInterest(%" PRI_PitEntrySerial ") pending=%" PRI_FaceId " [%s] flood=%" PRIuMAX, ie->serial(), downstream->faceid(), debug_list.c_str(), static_cast<uintmax_t>(flood_actual.count()));
  
  this->global()->scheduler()->Schedule(flood_actual, [this,ie](void){
    this->StartFlood(ie);
    return Scheduler::kNoMore;
  }, &ie->ie()->strategy.ev, true);
}

void SelfLearnStrategy::SendInterest(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, Ptr<PitUpstreamRecord> upstream) {
  Ptr<NamePrefixEntry> npe = ie->npe();
  FaceId upstream_face = upstream->faceid();
  NpeExtra* extra = this->GetExtra(npe);
  auto it = extra->predicts_.find(upstream_face);
  if (it != extra->predicts_.end()) {
    this->global()->scheduler()->Schedule(std::chrono::microseconds(it->second.time_), [this,ie,upstream_face](void){
      this->DidnotArriveOnFace(ie, upstream_face);
      return Scheduler::kNoMore;
    }, &upstream->p()->strategy_ev, true);
  }
  extra->interest_sent_time_[upstream_face] = CCNDH->wtnow;
  this->Strategy::SendInterest(ie, downstream, upstream);
}

void SelfLearnStrategy::StartFlood(Ptr<PitEntry> ie) {
  // Every upstream tried so far is listed in the PIT entry.
  // That list might be empty, if there's no FIB entry for the prefix.
  // We need to figure out which faces to broadcast, set expiry time to zero,
  // so that DoPropagate will forward to them all at once.

  // collect downstreams and upstreams
  std::vector<Ptr<Face>> downstreams;
  std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    Ptr<Face> downstream = this->global()->facemgr()->GetFace(p->faceid());
    if (downstream != nullptr) downstreams.push_back(downstream);
  });
  std::unordered_map<FaceId, Ptr<Face>> upstreams;
  std::for_each(ie->beginUpstream(), ie->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
    Ptr<Face> upstream = this->global()->facemgr()->GetFace(p->faceid());
    if (upstream != nullptr) upstreams[upstream->id()] = upstream;
  });
  
  std::string debug_list; char debug_buf[32];
#define DEBUG_APPEND_FaceId(x) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ",", x); debug_list.append(debug_buf); }

  // find faces to broadcast
  std::vector<Ptr<Face>> mcast_outbound, unicast_outbound;
  std::unordered_set<FaceId> outbounds;
  for (Ptr<Face> face : *this->global()->facemgr()) {
    if (!face->CanSend() || upstreams.count(face->id()) != 0) {
      // exclude no-send faces, and faces already tried earlier
      continue;
    }
    switch (face->kind()) {
      case FaceKind::kInternal:
      case FaceKind::kApp:
        // exclude local faces: they always register prefix
        continue;
      case FaceKind::kMulticast:
        mcast_outbound.push_back(face);
        break;
      case FaceKind::kUnicast:
        unicast_outbound.push_back(face);
        break;
      default:
        assert(false);
    }
  };
  
  // exclude mcast group that includes unicast downstream
  for (Ptr<Face> face : mcast_outbound) {
    bool reach = false;
    for (Ptr<Face> downstream : downstreams) {
      if (face->SendReachable(downstream)) {
        reach = true;
        break;
      }
    }
    if (reach) continue;
    outbounds.insert(face->id());
    DEBUG_APPEND_FaceId(face->id());
  }
  
  // exclude unicast peers reachable on mcast group
  for (Ptr<Face> face : unicast_outbound) {
    bool reach = false;
    for (auto pair : upstreams) {
      Ptr<Face> upstream = pair.second;
      if (upstream->SendReachable(face)) {
        reach = true;
        break;
      }
    }
    if (reach) continue;
    for (Ptr<Face> upstream : mcast_outbound) {
      if (upstream->SendReachable(face)) {
        reach = true;
        break;
      }
    }
    if (reach) continue;
    outbounds.insert(face->id());
    DEBUG_APPEND_FaceId(face->id());
  }
  
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
#undef DEBUG_APPEND_FaceId
  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::StartFlood(%" PRI_PitEntrySerial ") broadcast=[%s]", ie->serial(), debug_list.c_str());

  if (outbounds.empty()) return;
  
  this->PopulateOutbounds(ie, outbounds);
  this->global()->scheduler()->Schedule(ie->NextEventDelay(true), std::bind(&Strategy::DoPropagate, this, ie), &ie->ie()->ev, true);
}

void SelfLearnStrategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {
  if (matching_suffix == 0) {
    npe->ForeachPit([&] (Ptr<PitEntry> ie) ->ForeachAction {
      Ptr<PitUpstreamRecord> upstream = ie->GetUpstream(co->incoming_face());
      if (upstream != nullptr) this->global()->scheduler()->Cancel(upstream->p()->strategy_ev);
      FOREACH_OK;
    });
  }

  Ptr<Face> inface = this->global()->facemgr()->GetFace(co->incoming_face());
  if (inface == nullptr) return;
  Ptr<Face> peer = inface;
  if (inface->kind() == FaceKind::kMulticast) {
    peer = this->global()->facemgr()->MakeUnicastFace(inface, co->incoming_sender());
  }
  
  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::DidSatisfyPendingInterests(%s) upstream=%" PRI_FaceId " peer=%" PRI_FaceId " matching_suffix=%d", npe->name()->ToUri().c_str(), inface->id(), peer->id(), matching_suffix);
  
  NpeExtra* extra = this->GetExtra(npe);
  //PredictRecord& pr = extra->predicts_[peer->id()];
  //if (pr.time_.count() == 0) {
  //  pr.time_ = SelfLearnStrategy::initial_prediction();
  //}
  //pr.time_ = std::max(std::chrono::microseconds(127), (pr.time_ - pr.time_ / 128));
  
  auto it_interest_time = extra->interest_sent_time_.find(inface->id());
  if (it_interest_time != extra->interest_sent_time_.end()) {
    std::chrono::microseconds rtt_sample((CCNDH->wtnow - it_interest_time->second) * (1000000 / WTHZ_value()));
    this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::DidSatisfyPendingInterests(%s) rtt_sample[%" PRI_FaceId "]=%" PRIuMAX "", npe->name()->ToUri().c_str(), peer->id(), static_cast<uintmax_t>(rtt_sample.count()));
    for (Ptr<NamePrefixEntry> npe1 = npe; npe1 != nullptr; npe1 = npe1->Parent()) {
      NpeExtra* extra1 = this->GetExtra(npe1);
      PredictRecord& pr = extra1->predicts_[peer->id()];
      pr.rtt_.Measurement(rtt_sample);
    }
  }
}

void SelfLearnStrategy::DidnotArriveOnFace(Ptr<PitEntry> ie, FaceId face) {
  this->Log(kLLDebug, kLCStrategy, "SelfLearnStrategy::DidnotArriveOnFace(%" PRI_PitEntrySerial ",%" PRI_FaceId ")", ie->serial(), face);

  for (Ptr<NamePrefixEntry> npe1 = ie->npe(); npe1 != nullptr; npe1 = npe1->Parent()) {
    NpeExtra* extra = this->GetExtra(npe1);
    PredictRecord& pr = extra->predicts_[face];
    //if (pr.time_.count() == 0) {
    //  pr.time_ = SelfLearnStrategy::initial_prediction();
    //}
    //pr.time_ = std::min(std::chrono::microseconds(160000), (pr.time_ + pr.time_ / 8));
    pr.rtt_.IncreaseMultiplier();
  }
}

void SelfLearnStrategy::FinalizeNpeExtra(void* extra) {
  if (extra == nullptr) return;
  NpeExtra* extra1 = reinterpret_cast<NpeExtra*>(extra);
  delete extra1;
}

};//namespace ndnfd
