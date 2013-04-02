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
}

void Strategy::PropagateInterest(Ptr<InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  ccnd_handle* h = this->global()->ccndh();
  Ptr<PitEntry> ie = this->global()->npt()->SeekPit(interest, npe);
  bool is_new_ie = ie->ie()->strategy.renewals == 0;
  Ptr<Face> inface = this->global()->facemgr()->GetFace(interest->incoming_face());
  assert(inface != nullptr);
  
  // read nonce from Interest, or generate one.
  const uint8_t* nonce; size_t nonce_size; bool is_nonce_generated; uint8_t generated_nonce_buf[TYPICAL_NONCE_SIZE];
  if (interest->parsed()->offset[CCN_PI_B_Nonce] != interest->parsed()->offset[CCN_PI_E_Nonce]) {
    is_nonce_generated = false;
    ccn_ref_tagged_BLOB(CCN_DTAG_Nonce, interest->msg(), interest->parsed()->offset[CCN_PI_B_Nonce], interest->parsed()->offset[CCN_PI_E_Nonce], &nonce, &nonce_size);
  } else {
    is_nonce_generated = true;
    nonce_size = (h->noncegen)(h, inface->ccnd_face(), generated_nonce_buf);
    nonce = generated_nonce_buf;
  }
  
  // verify whether nonce is unique
  pit_face_item* p = ie->SeekDownstream(interest->incoming_face());
  p = pfi_set_nonce(h, ie->ie(), p, nonce, nonce_size);
  // TODO discuss whether it's correct for pfi_unique_nonce to consider outgoing nonce and expired pfi
  if (is_new_ie || is_nonce_generated || pfi_unique_nonce(h, ie->ie(), p)) {
    // unique nonce
    ie->ie()->strategy.renewed = h->wtnow;
    ie->ie()->strategy.renewals += 1;
    if ((p->pfi_flags & CCND_PFI_PENDING) == 0) {
      p->pfi_flags |= CCND_PFI_PENDING;
      inface->ccnd_face()->pending_interests += 1;
    }
  } else {
    // duplicate nonce
    // record inface as a downstream, but don't send ContentObject (because lack of CCND_PFI_PENDING flag)
    p->pfi_flags |= CCND_PFI_SUPDATA;
    this->Log(kLLDebug, kLCStrategy, "Strategy::PropagateInterest(%" PRI_PitEntrySerial ") duplicate nonce from %" PRI_FaceId "", ie->serial(), interest->incoming_face());
  }
  
  // set expiry time according to InterestLifetime
  pfi_set_expiry_from_lifetime(h, ie->ie(), p, ccn_interest_lifetime(interest->msg(), interest->parsed()));
  
  // lookup FIB and populate upstream pfi
  std::unordered_set<FaceId> outbounds = this->LookupOutbounds(npe, interest, ie);
  this->PopulateOutbounds(ie, outbounds);
  
  // schedule DoPropagate
  if (is_new_ie) {
    this->PropagateNewInterest(ie);
  }
  std::chrono::microseconds next_evt = outbounds.empty() ? std::chrono::microseconds(0) : ie->NextEventDelay(true);
  this->global()->scheduler()->Schedule(next_evt, std::bind(&Strategy::DoPropagate, this, ie), &ie->ie()->ev, true);
}

std::unordered_set<FaceId> Strategy::LookupOutbounds(Ptr<NamePrefixEntry> npe, Ptr<InterestMessage> interest, Ptr<PitEntry> ie) {
  return npe->LookupFib(interest);
}

void Strategy::PopulateOutbounds(Ptr<PitEntry> ie, const std::unordered_set<FaceId>& outbounds) {
  ccnd_handle* h = this->global()->ccndh();
  for (FaceId face : outbounds) {
    pit_face_item* p = ie->SeekUpstream(face);
    if (wt_compare(p->expiry, h->wtnow) < 0) {
      p->expiry = h->wtnow + 1;
      p->pfi_flags &= ~CCND_PFI_UPHUNGRY;
    }
  }
}

void Strategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  Ptr<NamePrefixEntry> npe = ie->npe();

  // find downstream
  pit_face_item* downstream = nullptr;
  ie->ForeachDownstream([&] (pit_face_item* p) ->ForeachAction {
    downstream = p;
    FOREACH_BREAK;
  });
  assert(downstream != nullptr && (downstream->pfi_flags & CCND_PFI_PENDING) != 0);
  
  // get tap list: Interest is immediately sent to these faces; they are used for monitoring purpose and shouldn't respond
  ccn_indexbuf* tap = nullptr;
  Ptr<NamePrefixEntry> npe_fib = npe->FibNode();
  if (npe_fib != nullptr) {
    tap = npe_fib->npe()->tap;
  }
  
  // read the known best face
  unsigned best = npe->npe()->src;
  if (best == CCN_NOFACEID) {
    best = npe->npe()->src = npe->npe()->osrc;
  }
  bool use_first;// whether to send interest immediately to the first upstream
  uint32_t defer_min, defer_range;// defer time for all but best and tap faces is within [defer_min,defer_min+defer_range) microseconds
  if (best == CCN_NOFACEID) {
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
#define DEBUG_APPEND_FaceTime(face,s,time) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ":%s%" PRIu32 ",", static_cast<FaceId>(face),s,time); debug_list.append(debug_buf); }
  
  // send interest to best and tap faces,
  // set expiry time for first face if use_first, and previous best face,
  // mark other faces as CCND_PFI_SENDUPST and ++n_upst
  int n_upst = 0;
  ie->ForeachUpstream([&] (pit_face_item* p) ->ForeachAction {
    if (p->faceid == best) {
      this->global()->scheduler()->Schedule(std::chrono::microseconds(defer_min), [this,ie](void){
        adjust_predicted_response(this->global()->ccndh(), ie->ie(), 1);
        return Scheduler::kNoMore;
      }, &ie->ie()->strategy.ev, true);
      DEBUG_APPEND_FaceTime(p->faceid,"best+",defer_min);
      this->SendInterest(ie, static_cast<FaceId>(downstream->faceid), static_cast<FaceId>(p->faceid));
      p = nullptr;//SendInterest might relocate p
    } else if (ccn_indexbuf_member(tap, p->faceid) >= 0) {
      DEBUG_APPEND_FaceTime(p->faceid,"tap+",0);
      this->SendInterest(ie, static_cast<FaceId>(downstream->faceid), static_cast<FaceId>(p->faceid));
      p = nullptr;//SendInterest might relocate p
    } else if (use_first) {
      use_first = false;
      // DoPropagate will send interest to this face;
      // since we don't know who is the best, just pick the first one
      pfi_set_expiry_from_micros(this->global()->ccndh(), ie->ie(), p, 0);
      DEBUG_APPEND_FaceTime(p->faceid,"first",0);
    } else if (p->faceid == npe->npe()->osrc) {
      pfi_set_expiry_from_micros(this->global()->ccndh(), ie->ie(), p, defer_min);
      DEBUG_APPEND_FaceTime(p->faceid,"osrc",defer_min);
    } else {
      ++n_upst;
      p->pfi_flags |= CCND_PFI_SENDUPST;
    }
    FOREACH_OK;
  });

  if (n_upst > 0) {
    uint32_t defer_max_inc = std::max(1U, (2 * defer_range + n_upst - 1) / n_upst);// max increment between defer times
    uint32_t defer = defer_min;
    ie->ForeachUpstream([&] (pit_face_item* p) ->ForeachAction {
      if ((p->pfi_flags & CCND_PFI_SENDUPST) == 0) FOREACH_CONTINUE;
      pfi_set_expiry_from_micros(this->global()->ccndh(), ie->ie(), p, defer);
      DEBUG_APPEND_FaceTime(p->faceid,"",defer);
      defer += nrand48(this->global()->ccndh()->seed) % defer_max_inc;
      FOREACH_OK;
    });
  }

#undef DEBUG_APPEND_FaceTime
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  this->Log(kLLDebug, kLCStrategy, "Strategy::PropagateNewInterest(%" PRI_PitEntrySerial ") {%s}", ie->serial(), debug_list.c_str());
}

std::chrono::microseconds Strategy::DoPropagate(Ptr<PitEntry> ie) {
  assert(ie != nullptr);
  ccn_wrappedtime now = this->global()->ccndh()->wtnow;
  
  std::string debug_list("pending=["); char debug_buf[32];
#define DEBUG_APPEND_FaceId(x) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ",", static_cast<FaceId>(x)); debug_list.append(debug_buf); }

  // find pending downstreams
  int pending = 0;//count of pending downstreams
  std::vector<pit_face_item*> downstreams;//pending downstreams that does not expire soon
  ie->ForeachDownstream([&] (pit_face_item* p) ->ForeachAction {
    if (wt_compare(p->expiry, now) <= 0) {// expired
      return ForeachAction::kDelete;
    }
    if ((p->pfi_flags & CCND_PFI_PENDING) == 0) {
      FOREACH_CONTINUE;
    }
    ++pending;
    DEBUG_APPEND_FaceId(p->faceid);
    if ((p->expiry - now) * 8 <= (p->expiry - p->renewed)) {// will expire soon (less than 1/8 remaining lifetime)
      FOREACH_CONTINUE;
    }
    downstreams.push_back(p);
    FOREACH_OK;
  });
  // keep at most two downstreams with longest lifetime; interests will be sent on their behalf (with their nonces)
  if (downstreams.size() > 2) {
    std::partial_sort(downstreams.begin(), downstreams.begin()+2, downstreams.end());
    downstreams.resize(2);
  }
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  debug_list.append("] upstreams=[");
  
  int upstreams = 0;//count of unexpired upstreams
  ie->ForeachUpstream([&] (pit_face_item* p) ->ForeachAction {
    Ptr<Face> face = this->global()->facemgr()->GetFace(static_cast<FaceId>(p->faceid));
    if (face == nullptr || !face->CanSend() || face->send_blocked()) {// cannot send on face
      return ForeachAction::kDelete;
    }
    if (wt_compare(now+1, p->expiry) < 0) {// not expired: an Interest was sent earlier
      ++upstreams;
      DEBUG_APPEND_FaceId(p->faceid);
      FOREACH_CONTINUE;
    }
    // find a downstream that is different from this upstream; Interest will be sent with that nonce
    FaceId interest_from = FaceId_none;
    for (pit_face_item* downstream : downstreams) {
      if (downstream->faceid != p->faceid) {
        interest_from = static_cast<FaceId>(downstream->faceid);
        break;
      }
    }
    if (interest_from != FaceId_none) {
      debug_list.push_back('+');
      DEBUG_APPEND_FaceId(p->faceid);
      this->SendInterest(ie, interest_from, static_cast<FaceId>(p->faceid));
      p = nullptr;// don't use p anymore: send_interest may reallocate it
      ++upstreams;
    } else {
      p->pfi_flags |= CCND_PFI_UPHUNGRY;
    }
    FOREACH_OK;
  });
  
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  debug_list.append("]");
#undef DEBUG_APPEND_FaceId
  this->Log(kLLDebug, kLCStrategy, "Strategy::DoPropagate(%" PRI_PitEntrySerial ") %s", ie->serial(), debug_list.c_str());

  bool include_expired_in_next_evt = false;
  if (upstreams == 0) {
    if (pending == 0) {
      this->WillEraseTimedOutPendingInterest(ie);
      ie->ie()->ev = nullptr;
      this->global()->npt()->DeletePit(ie);
      ie = nullptr;
      return Scheduler::kNoMore;
    }
    include_expired_in_next_evt = this->DidExhaustForwardingOptions(ie);
  }
  
  return ie->NextEventDelay(include_expired_in_next_evt);
}

void Strategy::SendInterest(Ptr<PitEntry> ie, FaceId downstream, FaceId upstream) {
  //this->Log(kLLDebug, kLCStrategy, "Strategy::SendInterest(%" PRI_PitEntrySerial ") %" PRI_FaceId " => %" PRI_FaceId "", ie->serial(), downstream, upstream);
  pit_face_item* downstream_pfi = ie->GetDownstream(downstream);
  pit_face_item* upstream_pfi = ie->GetUpstream(upstream);
  assert(downstream_pfi != nullptr);
  assert(upstream_pfi != nullptr);
  send_interest(this->global()->ccndh(), ie->ie(), downstream_pfi, upstream_pfi);
}

bool Strategy::DidExhaustForwardingOptions(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::DidExhaustForwardingOptions(%" PRI_PitEntrySerial ")", ie->serial());
  return false;
}

void Strategy::WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillEraseTimedOutPendingInterest(%" PRI_PitEntrySerial ")", ie->serial());
}

void Strategy::WillSatisfyPendingInterest(Ptr<PitEntry> ie, FaceId upstream) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillSatisfyPendingInterest(%" PRI_PitEntrySerial ") upstream=%" PRI_FaceId "", ie->serial(), upstream);
}

void Strategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, FaceId upstream) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::DidSatisfyPendingInterests(%s) upstream=%" PRI_FaceId "", npe->name()->ToUri().c_str(), upstream);
  int limit = 2;
  for (; npe != nullptr && --limit >= 0; npe = npe->Parent()) {
    if (npe->npe()->src == static_cast<unsigned>(upstream)) {
      adjust_npe_predicted_response(this->global()->ccndh(), npe->npe(), 0);
      continue;
    }
    if (npe->npe()->src != CCN_NOFACEID) {
      npe->npe()->osrc = npe->npe()->src;
    }
    npe->npe()->src = static_cast<unsigned>(upstream);
  }
}

void Strategy::DidAddFibEntry(Ptr<ForwardingEntry> forw) {
  assert(forw != nullptr);
  Ptr<NamePrefixEntry> npe = forw->npe();
  FaceId faceid = forw->face();
  
  std::chrono::microseconds defer(6000);

  npe->ForeachPit([&] (Ptr<PitEntry> ie) ->ForeachAction {
    if (ie->GetUpstream(faceid) != nullptr) FOREACH_CONTINUE;
    
    Ptr<Face> downstream = nullptr;
    ie->ForeachDownstream([&] (pit_face_item* p) ->ForeachAction {
      if (downstream == nullptr || downstream->kind() != FaceKind::kApp) {
        downstream = this->global()->facemgr()->GetFace(static_cast<FaceId>(p->faceid));
      }
      FOREACH_OK;
    });
    if (downstream == nullptr) FOREACH_CONTINUE;
    
    Ptr<const InterestMessage> interest = ie->interest();
    std::unordered_set<FaceId> outbound = npe->LookupFib(interest);
    if (outbound.find(faceid) == outbound.end()) FOREACH_CONTINUE;
    
    pit_face_item* p = ie->SeekUpstream(faceid);
    if ((p->pfi_flags & CCND_PFI_UPENDING) != 0) FOREACH_CONTINUE;
    
    p->expiry = this->global()->ccndh()->wtnow + defer.count() / (1000000 / WTHZ_value());
    defer += std::chrono::microseconds(200);
    this->global()->scheduler()->Schedule(defer, std::bind(&Strategy::DoPropagate, this, ie), &ie->ie()->ev, true);
    FOREACH_OK;
  });
}

};//namespace ndnfd
