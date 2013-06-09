#include "original.h"
#include <algorithm>
#include "face/facemgr.h"
namespace ndnfd {

void OriginalStrategy::Init2(void) {
  this->agebestface_evt_ = this->global()->scheduler()->Schedule(std::chrono::microseconds(8000000), std::bind(&OriginalStrategy::AgeBestFace, this));
}

OriginalStrategy::~OriginalStrategy(void) {
  this->global()->scheduler()->Cancel(this->agebestface_evt_);
}

void OriginalStrategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  Ptr<NamePrefixEntry> npe = ie->npe();
  NpeExtra* extra = npe->strategy_extra<NpeExtra>();

  // find downstream
  auto first_downstream = ie->beginDownstream();
  assert(first_downstream != ie->endDownstream());
  Ptr<PitDownstreamRecord> downstream = *first_downstream;
  assert(downstream != nullptr && downstream->pending());
  
  // get tap list: Interest is immediately sent to these faces; they are used for monitoring purpose and shouldn't respond
  ccn_indexbuf* tap = nullptr;
  Ptr<NamePrefixEntry> npe_fib = npe->FibNode();
  if (npe_fib != nullptr) {
    tap = npe_fib->native()->tap;
  }
  
  // read the known best face
  FaceId best = extra->GetBestFace();
  bool use_first;// whether to send interest immediately to the first upstream
  uint32_t defer_min, defer_range;// defer time for all but best and tap faces is within [defer_min,defer_min+defer_range) microseconds
  if (best == FaceId_none) {
    use_first = true;
    defer_min = 4000;
    defer_range = 75000;
  } else {
    use_first = false;
    defer_min = static_cast<uint32_t>(extra->prediction_.count());
    defer_range = (defer_min + 1) / 2;
  }

  std::string debug_list;
  char debug_buf[32];
#define DEBUG_APPEND_FaceTime(face,s,time) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ":%s%" PRIu32 ",", face,s,time); debug_list.append(debug_buf); }
  
  // send interest to best and tap faces,
  // set expiry time for first face if use_first, and previous best face,
  // mark other faces as CCND_PFI_SENDUPST and ++n_upst
  int n_upst = 0;
  std::for_each(ie->beginUpstream(), ie->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
    if (p->faceid() == best) {
      this->global()->scheduler()->Schedule(std::chrono::microseconds(defer_min), [this,ie,best](void){
        // don't assert(ie->npe()->best_faceid() == best), src may be aged out and set to CCN_NOFACEID
        this->DidnotArriveOnBestFace(ie);
        return Scheduler::kNoMore;
      }, &ie->native()->strategy.ev, true);
      DEBUG_APPEND_FaceTime(p->faceid(),"best+",defer_min);
      this->SendInterest(ie, downstream, p);
    } else if (ccn_indexbuf_member(tap, p->faceid()) >= 0) {
      DEBUG_APPEND_FaceTime(p->faceid(),"tap+",0);
      this->SendInterest(ie, downstream, p);
    } else if (use_first) {
      use_first = false;
      // DoPropagate will send interest to this face;
      // since we don't know who is the best, just pick the first one
      p->SetExpiry(std::chrono::microseconds::zero());
      DEBUG_APPEND_FaceTime(p->faceid(),"first",0);
    } else if (p->faceid() == extra->prev_faceid_) {
      p->SetExpiry(std::chrono::microseconds(defer_min));
      DEBUG_APPEND_FaceTime(p->faceid(),"osrc",defer_min);
    } else {
      ++n_upst;
      p->native()->pfi_flags |= CCND_PFI_SENDUPST;
    }
  });

  if (n_upst > 0) {
    uint32_t defer_max_inc = std::max(1U, (2 * defer_range + n_upst - 1) / n_upst);// max increment between defer times
    uint32_t defer = defer_min;
    std::for_each(ie->beginUpstream(), ie->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
      if ((p->native()->pfi_flags & CCND_PFI_SENDUPST) == 0) return;
      p->SetExpiry(std::chrono::microseconds(defer));
      DEBUG_APPEND_FaceTime(p->faceid(),"",defer);
      defer += nrand48(CCNDH->seed) % defer_max_inc;
    });
  }

#undef DEBUG_APPEND_FaceTime
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  this->Log(kLLDebug, kLCStrategy, "OriginalStrategy::PropagateNewInterest(%" PRI_PitEntrySerial ") best=%" PRI_FaceId " pending=%" PRI_FaceId " [%s]", ie->serial(), best, downstream->faceid(), debug_list.c_str());
}

void OriginalStrategy::DidnotArriveOnBestFace(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "OriginalStrategy::DidnotArriveOnBestFace(%" PRI_PitEntrySerial ") face=%" PRI_FaceId "", ie->serial(), ie->npe()->strategy_extra<NpeExtra>()->best_faceid_);
  int limit = 2;
  for (Ptr<NamePrefixEntry> npe = ie->npe(); npe != nullptr && --limit >= 0; npe = npe->Parent()) {
    npe->strategy_extra<NpeExtra>()->AdjustPredictUp();
  }
}

void OriginalStrategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {
  FaceId upstream = co->incoming_face();
  this->Log(kLLDebug, kLCStrategy, "OriginalStrategy::DidSatisfyPendingInterests(%s) upstream=%" PRI_FaceId " matching_suffix=%d", npe->name()->ToUri().c_str(), upstream, matching_suffix);
  if (matching_suffix >= 0 && matching_suffix < 2) npe->strategy_extra<NpeExtra>()->UpdateBestFace(upstream);
}

void OriginalStrategy::DidAddFibEntry(Ptr<ForwardingEntry> forw) {
  assert(forw != nullptr);
  Ptr<NamePrefixEntry> npe = forw->npe();
  FaceId faceid = forw->face();
  this->Log(kLLDebug, kLCStrategy, "OriginalStrategy::DidAddFibEntry(%s,%" PRI_FaceId ")", npe->name()->ToUri().c_str(), faceid);

  std::chrono::microseconds defer(6000);

  npe->ForeachPit([&] (Ptr<PitEntry> ie) ->ForeachAction {
    if (ie->GetUpstream(faceid) != nullptr) FOREACH_CONTINUE;
    
    Ptr<Face> downstream = nullptr;
    std::for_each(ie->beginDownstream(), ie->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
      if (downstream == nullptr || downstream->kind() != FaceKind::kApp) {
        downstream = this->global()->facemgr()->GetFace(p->faceid());
      }
    });
    if (downstream == nullptr) FOREACH_CONTINUE;
    
    Ptr<const InterestMessage> interest = ie->interest();
    std::unordered_set<FaceId> outbound = npe->LookupFib(interest);
    if (outbound.find(faceid) == outbound.end()) FOREACH_CONTINUE;
    
    Ptr<PitUpstreamRecord> p = ie->SeekUpstream(faceid);
    if (p->pending()) FOREACH_CONTINUE;
    
    p->SetExpiry(defer);
    defer += std::chrono::microseconds(200);
    this->global()->scheduler()->Schedule(defer, std::bind(&OriginalStrategy::DoPropagate, this, ie), &ie->native()->ev, true);
    FOREACH_OK;
  });
}

void OriginalStrategy::NewNpeExtra(Ptr<NamePrefixEntry> npe) {
  npe->set_strategy_extra(new NpeExtra());
}

void OriginalStrategy::InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent) {
  NpeExtra* parent_extra = parent->strategy_extra<NpeExtra>();
  npe->set_strategy_extra(new NpeExtra(*parent_extra));
}

void OriginalStrategy::FinalizeNpeExtra(Ptr<NamePrefixEntry> npe) {
  NpeExtra* extra = npe->strategy_extra<NpeExtra>();
  npe->set_strategy_extra<NpeExtra>(nullptr);
  delete extra;
}

std::chrono::microseconds OriginalStrategy::AgeBestFace(void) {
  int count = 0;
  this->global()->npt()->ForeachNpe([&] (Ptr<NamePrefixEntry> npe) ->ForeachAction {
    npe->strategy_extra<NpeExtra>()->AgeBestFace();
    ++count;
    FOREACH_OK;
  });
  this->Log(kLLDebug, kLCStrategy, "OriginalStrategy::AgeBestFace() count=%d", count);
  return std::chrono::microseconds(8000000);
}

OriginalStrategy::NpeExtra::NpeExtra(void) {
  this->best_faceid_ = this->prev_faceid_ = FaceId_none;
  this->prediction_ = std::chrono::microseconds(8192);
}

FaceId OriginalStrategy::NpeExtra::GetBestFace(void) {
  FaceId best = this->best_faceid_;
  if (best == FaceId_none) {
    best = this->prev_faceid_;
    this->best_faceid_ = best;
  }
  return best;
}

void OriginalStrategy::NpeExtra::UpdateBestFace(FaceId value) {
  if (value == FaceId_none) {
    this->prev_faceid_ = this->best_faceid_ = value;
    return;
  }
  if (this->best_faceid_ == value) {
    std::chrono::microseconds::rep t = this->prediction_.count();
    t = std::max(static_cast<std::chrono::microseconds::rep>(127), t - (t>>7));
    this->prediction_ = std::chrono::microseconds(t);
  } else if (this->best_faceid_ == FaceId_none) {
    this->best_faceid_ = value;
  } else {
    this->prev_faceid_ = this->best_faceid_;
    this->best_faceid_ = value;
  }
}

void OriginalStrategy::NpeExtra::AdjustPredictUp(void) {
  std::chrono::microseconds::rep t = this->prediction_.count();
  t = std::min(static_cast<std::chrono::microseconds::rep>(160000), t + (t>>3));
  this->prediction_ = std::chrono::microseconds(t);
}

void OriginalStrategy::NpeExtra::AgeBestFace(void) {
  this->prev_faceid_ = this->best_faceid_;
  this->best_faceid_ = FaceId_none;
}

};//namespace ndnfd
