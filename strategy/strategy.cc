#include "strategy.h"
#include <algorithm>
extern "C" {
struct pit_face_item* send_interest(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* x, struct pit_face_item* p);
struct pit_face_item* pfi_set_nonce(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, const uint8_t* nonce, size_t noncesize);
int pfi_unique_nonce(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p);
void pfi_set_expiry_from_lifetime(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, intmax_t lifetime);
uint32_t WTHZ_value(void);
int wt_compare(ccn_wrappedtime a, ccn_wrappedtime b);
}
#include "core/scheduler.h"
#include "face/facemgr.h"
namespace ndnfd {

void Strategy::Init(void) {
  this->ccnd_strategy_interface_ = this->New<CcndStrategyInterface>();
}

void Strategy::SendInterest(Ptr<PitEntry> ie, FaceId downstream, FaceId upstream) {
  //this->Log(kLLDebug, kLCStrategy, "Strategy::SendInterest(%" PRI_PitEntrySerial ") %" PRI_FaceId " => %" PRI_FaceId "", ie->serial(), downstream, upstream);
  send_interest(this->global()->ccndh(), ie->ie(), ie->SeekDownstream(downstream), ie->SeekUpstream(upstream));
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
    this->Log(kLLDebug, kLCStrategy, "Strategy::PropagateInterest(%" PRI_PitEntrySerial ") duplicate nonce", ie->serial());
  }
  
  // set expiry time according to InterestLifetime
  pfi_set_expiry_from_lifetime(h, ie->ie(), p, ccn_interest_lifetime(interest->msg(), interest->parsed()));
  
  // lookup FIB and populate upstream pfi
  std::unordered_set<FaceId> outbounds = npe->LookupFib(interest);
  for (FaceId face : outbounds) {
    pit_face_item* upstream = ie->SeekUpstream(face);
    if (wt_compare(upstream->expiry, h->wtnow) < 0) {
      p->expiry = h->wtnow + 1;
      p->pfi_flags &= ~CCND_PFI_UPHUNGRY;
    }
  }
  
  // strategy_callout / do_propagate
  if (is_new_ie) {
    this->PropagateNewInterest(ie);
  }
  std::chrono::microseconds next_evt = outbounds.empty() ? std::chrono::microseconds(0) : ie->NextEventDelay(true);
  this->global()->scheduler()->Cancel(ie->ie()->ev);
  ie->ie()->ev = this->global()->scheduler()->Schedule(next_evt, std::bind(&Strategy::DoPropagate, this, ie));
}

void Strategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::PropagateNewInterest(%" PRI_PitEntrySerial ")", ie->serial());
}

std::chrono::microseconds Strategy::DoPropagate(Ptr<PitEntry> ie) {
  assert(ie != nullptr);
  ccn_wrappedtime now = this->global()->ccndh()->wtnow;
  
  std::string debug_list("pending=[");
  char debug_buf[32];
#define DEBUG_APPEND_FaceId(x) { snprintf(debug_buf, sizeof(debug_buf), "%" PRI_FaceId ",", static_cast<FaceId>(x)); debug_list.append(debug_buf); }

  // find pending downstreams
  int pending = 0;//count of pending downstreams
  std::vector<pit_face_item*> downstreams;//pending downstreams that does not expire soon
  ie->ForeachDownstream([&] (pit_face_item* p) ->PitEntry::ForeachAction {
    if (wt_compare(p->expiry, now) <= 0) {// expired
      return PitEntry::ForeachAction::kDelete;
    }
    if ((p->pfi_flags & CCND_PFI_PENDING) == 0) {
      return PitEntry::ForeachAction::kNone;
    }
    ++pending;
    DEBUG_APPEND_FaceId(p->faceid);
    if ((p->expiry - now) * 8 <= (p->expiry - p->renewed)) {// will expire soon (less than 1/8 remaining lifetime)
      return PitEntry::ForeachAction::kNone;
    }
    downstreams.push_back(p);
    return PitEntry::ForeachAction::kNone;
  });
  // keep at most two downstreams with longest lifetime; interests will be sent on their behalf (with their nonces)
  if (downstreams.size() > 2) {
    std::partial_sort(downstreams.begin(), downstreams.begin()+2, downstreams.end());
    downstreams.resize(2);
  }
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  debug_list.append("] upstreams=[");
  
  int upstreams = 0;//count of unexpired upstreams
  ie->ForeachUpstream([&] (pit_face_item* p) ->PitEntry::ForeachAction {
    Ptr<Face> face = this->global()->facemgr()->GetFace(static_cast<FaceId>(p->faceid));
    if (face == nullptr || !face->CanSend() || face->send_blocked()) {// cannot send on face
      return PitEntry::ForeachAction::kDelete;
    }
    if (wt_compare(now+1, p->expiry) < 0) {// not expired: an Interest was sent earlier
      ++upstreams;
      DEBUG_APPEND_FaceId(p->faceid);
      return PitEntry::ForeachAction::kNone;
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
    return PitEntry::ForeachAction::kNone;
  });
  
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  debug_list.append("]");
#undef DEBUG_APPEND_FaceId
  this->Log(kLLDebug, kLCStrategy, "Strategy::DoPropagate(%" PRI_PitEntrySerial ") %s", ie->serial(), debug_list.c_str());

  if (upstreams == 0) {
    if (pending == 0) {
      this->WillEraseTimedOutPendingInterest(ie);
      ie->ie()->ev = nullptr;
      this->global()->npt()->DeletePit(ie);
      ie = nullptr;
      return Scheduler::kNoMore;
    }
    this->DidExhaustForwardingOptions(ie);
  }
  
  return ie->NextEventDelay(false);
}

void Strategy::DidExhaustForwardingOptions(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::DidExhaustForwardingOptions(%" PRI_PitEntrySerial ")", ie->serial());
}

void Strategy::WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillEraseTimedOutPendingInterest(%" PRI_PitEntrySerial ")", ie->serial());
}

void Strategy::WillSatisfyPendingInterest(Ptr<PitEntry> ie, FaceId upstream) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillSatisfyPendingInterest(%" PRI_PitEntrySerial ") upstream=%" PRI_FaceId "", ie->serial(), upstream);
}

void Strategy::DidAddFibEntry(Ptr<ForwardingEntry> forw) {
  assert(forw != nullptr);
  Ptr<NamePrefixEntry> npe = forw->npe();
  FaceId faceid = forw->face();
  
  std::chrono::microseconds defer(6000);

  npe->ForeachPit([&] (Ptr<PitEntry> ie) {
    if (ie->GetUpstream(faceid) != nullptr) return;
    
    Ptr<Face> downstream = nullptr;
    ie->ForeachDownstream([&] (pit_face_item* p) ->PitEntry::ForeachAction {
      if (downstream == nullptr || downstream->kind() != FaceKind::kApp) {
        downstream = this->global()->facemgr()->GetFace(static_cast<FaceId>(p->faceid));
      }
      return PitEntry::ForeachAction::kNone;
    });
    if (downstream == nullptr) return;
    
    Ptr<const InterestMessage> interest = ie->interest();
    std::unordered_set<FaceId> outbound = npe->LookupFib(interest);
    if (outbound.find(faceid) == outbound.end()) return;
    
    pit_face_item* p = ie->SeekUpstream(faceid);
    if ((p->pfi_flags & CCND_PFI_UPENDING) != 0) return;
    
    p->expiry = this->global()->ccndh()->wtnow + defer.count() / (1000000 / WTHZ_value());
    defer += std::chrono::microseconds(200);
    this->global()->scheduler()->Cancel(ie->ie()->ev);
    ie->ie()->ev = this->global()->scheduler()->Schedule(defer, std::bind(&Strategy::DoPropagate, this, ie));
  });
}

};//namespace ndnfd
