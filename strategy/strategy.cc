#include "strategy.h"
#include <algorithm>
extern "C" {
struct pit_face_item* send_interest(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* x, struct pit_face_item* p);
}
#include "core/scheduler.h"
#include "face/facemgr.h"
namespace ndnfd {

void Strategy::Init(void) {
  this->ccnd_strategy_interface_ = this->New<CcndStrategyInterface>();
  this->global()->npt()->FinalizeNpeExtra = std::bind(&Strategy::FinalizeNpeExtra, this, std::placeholders::_1);
}

void Strategy::PropagateInterest(Ptr<InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  Ptr<PitEntry> ie = this->global()->npt()->SeekPit(interest, npe);
  bool is_new_ie = ie->ie()->strategy.renewals == 0;
  Ptr<Face> inface = this->global()->facemgr()->GetFace(interest->incoming_face());
  assert(inface != nullptr);

#ifdef NDNFD_STRATEGY_TRACE
  switch (inface->kind()) {
    case FaceKind::kMulticast:
      this->Trace(TraceEvt::kMcastRecv, interest->name());
      break;
    case FaceKind::kUnicast:
      this->Trace(TraceEvt::kUnicastRecv, interest->name());
      break;
    default: break;
  }
#endif
  
  Ptr<PitDownstreamRecord> p = ie->SeekDownstream(inface->id());
  // read nonce from Interest, or generate one.
  p->UpdateNonce(interest);
  // verify whether nonce is unique
  if (ie->IsNonceUnique(p)) {
    // unique nonce
    ie->ie()->strategy.renewed = CCNDH->wtnow;
    ie->ie()->strategy.renewals += 1;
    if (!p->pending()) {
      p->set_pending(true);
      inface->ccnd_face()->pending_interests += 1;
    }
  } else {
    // duplicate nonce
    // record inface as a downstream, but don't forward Interest (because lack of pending flag)
    p->set_suppress(true);
    this->Log(kLLDebug, kLCStrategy, "Strategy::PropagateInterest(%" PRI_PitEntrySerial ") duplicate nonce from %" PRI_FaceId "", ie->serial(), interest->incoming_face());
  }
  
  // set expiry time according to InterestLifetime
  p->SetExpiryToLifetime(interest);
  
  // lookup FIB and populate upstream pfi
  std::unordered_set<FaceId> outbounds = this->LookupOutbounds(ie, interest);
  this->PopulateOutbounds(ie, outbounds);
  
  // schedule DoPropagate
  if (is_new_ie) {
    this->PropagateNewInterest(ie);
  }
  std::chrono::microseconds next_evt = outbounds.empty() ? std::chrono::microseconds(0) : ie->NextEventDelay(true);
  this->global()->scheduler()->Schedule(next_evt, std::bind(&Strategy::DoPropagate, this, ie), &ie->ie()->ev, true);
}

std::unordered_set<FaceId> Strategy::LookupOutbounds(Ptr<PitEntry> ie, Ptr<InterestMessage> interest) {
  return ie->npe()->LookupFib(interest);
}

void Strategy::PopulateOutbounds(Ptr<PitEntry> ie, const std::unordered_set<FaceId>& outbounds) {
  for (FaceId face : outbounds) {
    Ptr<PitUpstreamRecord> p = ie->SeekUpstream(face);
    if (p->IsExpired()) {
      p->SetExpiry(std::chrono::microseconds::zero());
      p->p()->pfi_flags &= ~CCND_PFI_UPHUNGRY;
    }
  }
}

void Strategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  Ptr<NamePrefixEntry> npe = ie->npe();

  // find downstream
  auto first_downstream = ie->beginDownstream();
  assert(first_downstream != ie->endDownstream());
  Ptr<PitDownstreamRecord> downstream = *first_downstream;
  assert(downstream != nullptr && downstream->pending());
  
  // get tap list: Interest is immediately sent to these faces; they are used for monitoring purpose and shouldn't respond
  ccn_indexbuf* tap = nullptr;
  Ptr<NamePrefixEntry> npe_fib = npe->FibNode();
  if (npe_fib != nullptr) {
    tap = npe_fib->npe()->tap;
  }
  
  // read the known best face
  FaceId best = npe->GetBestFace();
  bool use_first;// whether to send interest immediately to the first upstream
  uint32_t defer_min, defer_range;// defer time for all but best and tap faces is within [defer_min,defer_min+defer_range) microseconds
  if (best == FaceId_none) {
    use_first = true;
    defer_min = 4000;
    defer_range = 75000;
  } else {
    use_first = false;
    defer_min = static_cast<uint32_t>(npe->npe()->usec);
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
      }, &ie->ie()->strategy.ev, true);
      DEBUG_APPEND_FaceTime(p->faceid(),"best+",defer_min);
      this->SendInterest(ie, downstream, p);
    } else if (ccn_indexbuf_member(tap, p->p()->faceid) >= 0) {
      DEBUG_APPEND_FaceTime(p->faceid(),"tap+",0);
      this->SendInterest(ie, downstream, p);
    } else if (use_first) {
      use_first = false;
      // DoPropagate will send interest to this face;
      // since we don't know who is the best, just pick the first one
      p->SetExpiry(std::chrono::microseconds::zero());
      DEBUG_APPEND_FaceTime(p->faceid(),"first",0);
    } else if (p->faceid() == npe->prev_faceid()) {
      p->SetExpiry(std::chrono::microseconds(defer_min));
      DEBUG_APPEND_FaceTime(p->faceid(),"osrc",defer_min);
    } else {
      ++n_upst;
      p->p()->pfi_flags |= CCND_PFI_SENDUPST;
    }
  });

  if (n_upst > 0) {
    uint32_t defer_max_inc = std::max(1U, (2 * defer_range + n_upst - 1) / n_upst);// max increment between defer times
    uint32_t defer = defer_min;
    std::for_each(ie->beginUpstream(), ie->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
      if ((p->p()->pfi_flags & CCND_PFI_SENDUPST) == 0) return;
      p->SetExpiry(std::chrono::microseconds(defer));
      DEBUG_APPEND_FaceTime(p->faceid(),"",defer);
      defer += nrand48(CCNDH->seed) % defer_max_inc;
    });
  }

#undef DEBUG_APPEND_FaceTime
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  this->Log(kLLDebug, kLCStrategy, "Strategy::PropagateNewInterest(%" PRI_PitEntrySerial ") best=%" PRI_FaceId " pending=%" PRI_FaceId " [%s]", ie->serial(), best, downstream->faceid(), debug_list.c_str());
}

std::chrono::microseconds Strategy::DoPropagate(Ptr<PitEntry> ie) {
  assert(ie != nullptr);
  Ptr<NamePrefixEntry> npe = ie->npe();
  ccn_wrappedtime now = CCNDH->wtnow;
  
  std::string debug_list("downstreams=["); char debug_buf[32];
#define DEBUG_APPEND_FaceId(c,p) { snprintf(debug_buf, sizeof(debug_buf), "%c%" PRI_FaceId ",", c, p->faceid()); debug_list.append(debug_buf); }

  // find pending downstreams
  int pending = 0;//count of pending downstreams
  std::vector<Ptr<PitDownstreamRecord>> downstreams;//pending downstreams that does not expire soon
  for (auto it = ie->beginDownstream(); it != ie->endDownstream(); ++it) {
    Ptr<PitDownstreamRecord> p = *it;
    if (p->IsExpired()) { 
      DEBUG_APPEND_FaceId('-',p);
      it.Delete();
      continue;
    }
    if (!p->pending()) continue;
    ++pending;
    if ((p->p()->expiry - now) * 8 <= (p->p()->expiry - p->p()->renewed)) {// will expire soon (less than 1/8 remaining lifetime)
      DEBUG_APPEND_FaceId('~',p);
      continue;
    }
    DEBUG_APPEND_FaceId('_',p);
    downstreams.push_back(p);
  };
  // keep at most two downstreams with longest lifetime; interests will be sent on their behalf (with their nonces)
  if (downstreams.size() > 2) {
    std::partial_sort(downstreams.begin(), downstreams.begin()+2, downstreams.end(),
      [] (const Ptr<PitDownstreamRecord>& a, const Ptr<PitDownstreamRecord>& b) ->bool {
        return PitDownstreamRecord::CompareExpiry(a, b) > 0;
      });
    downstreams.resize(2);
  }
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  debug_list.append("] upstreams=[");
  
  int upstreams = 0;//count of unexpired upstreams
  for (auto it = ie->beginUpstream(); it != ie->endUpstream(); ++it) {
    Ptr<PitUpstreamRecord> p = *it;
    Ptr<Face> face = this->global()->facemgr()->GetFace(p->faceid());
    if (face == nullptr || !face->CanSend() || face->send_blocked()) {// cannot send on face
      it.Delete();
      continue;
    }
    if (!p->IsExpired()) {
      if (p->pending()) {// Interest sent
        DEBUG_APPEND_FaceId('_',p);
      } else {// defer
        DEBUG_APPEND_FaceId('.',p);
      }
      ++upstreams;
      continue;
    }
    
    if (p->pending()) {// Interest sent but no response
      DEBUG_APPEND_FaceId('-',p);
      if (npe->best_faceid() == p->faceid() || npe->prev_faceid() == p->faceid()) npe->UpdateBestFace(FaceId_none);
      continue;// don't send another Interest
    }
    
    // find a downstream that is different from this upstream; Interest will be sent with that nonce
    Ptr<PitDownstreamRecord> interest_from = nullptr;
    for (Ptr<PitDownstreamRecord> downstream : downstreams) {
      if (downstream->faceid() != p->faceid()) {
        interest_from = downstream;
        break;
      }
    }
    if (interest_from != nullptr) {
      DEBUG_APPEND_FaceId('+',p);
      this->SendInterest(ie, interest_from, p);
      ++upstreams;
    } else {
      DEBUG_APPEND_FaceId('~',p);
      p->p()->pfi_flags |= CCND_PFI_UPHUNGRY;
    }
  }
  
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  debug_list.append("]");
#undef DEBUG_APPEND_FaceId
  this->Log(kLLDebug, kLCStrategy, "Strategy::DoPropagate(%" PRI_PitEntrySerial ") %s", ie->serial(), debug_list.c_str());

  if (upstreams == 0 && pending == 0) {
    this->WillEraseTimedOutPendingInterest(ie);
    ie->ie()->ev = nullptr;
    this->global()->npt()->DeletePit(ie);
    ie = nullptr;
    return Scheduler::kNoMore;
  }
  
  return ie->NextEventDelay(false);
}

void Strategy::SendInterest(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, Ptr<PitUpstreamRecord> upstream) {
  //this->Log(kLLDebug, kLCStrategy, "Strategy::SendInterest(%" PRI_PitEntrySerial ") %" PRI_FaceId " => %" PRI_FaceId "", ie->serial(), downstream, upstream);
  assert(ie != nullptr);
  assert(downstream != nullptr);
  assert(upstream != nullptr);
  upstream->set_p(send_interest(CCNDH, ie->ie(), downstream->p(), upstream->p()));

#ifdef NDNFD_STRATEGY_TRACE
  Ptr<Face> outface = this->global()->facemgr()->GetFace(upstream->faceid());
  if (outface != nullptr) {
    switch (outface->kind()) {
      case FaceKind::kMulticast:
        this->Trace(TraceEvt::kMcastSend, ie->name());
        break;
      case FaceKind::kUnicast:
        this->Trace(TraceEvt::kUnicastSend, ie->name());
        break;
      default: break;
    }
  }
#endif
}

void Strategy::DidnotArriveOnBestFace(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::DidnotArriveOnBestFace(%" PRI_PitEntrySerial ") face=%" PRI_FaceId "", ie->serial(), ie->npe()->best_faceid());
  int limit = 2;
  for (Ptr<NamePrefixEntry> npe1 = ie->npe(); npe1 != nullptr && --limit >= 0; npe1 = npe1->Parent()) {
    npe1->AdjustPredictUp();
  }
}

void Strategy::WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillEraseTimedOutPendingInterest(%" PRI_PitEntrySerial ")", ie->serial());
}

void Strategy::WillSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const Message> co) {
  FaceId upstream = co->incoming_face();
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillSatisfyPendingInterest(%" PRI_PitEntrySerial ") upstream=%" PRI_FaceId "", ie->serial(), upstream);
}

void Strategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {
  FaceId upstream = co->incoming_face();
  this->Log(kLLDebug, kLCStrategy, "Strategy::DidSatisfyPendingInterests(%s) upstream=%" PRI_FaceId " matching_suffix=%d", npe->name()->ToUri().c_str(), upstream, matching_suffix);
  if (matching_suffix >= 0 && matching_suffix < 2) npe->UpdateBestFace(upstream);
}

void Strategy::DidAddFibEntry(Ptr<ForwardingEntry> forw) {
  assert(forw != nullptr);
  Ptr<NamePrefixEntry> npe = forw->npe();
  FaceId faceid = forw->face();
  
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
    this->global()->scheduler()->Schedule(defer, std::bind(&Strategy::DoPropagate, this, ie), &ie->ie()->ev, true);
    FOREACH_OK;
  });
}

};//namespace ndnfd
