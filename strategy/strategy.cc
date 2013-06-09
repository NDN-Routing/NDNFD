#include "strategy.h"
#include <algorithm>
extern "C" {
struct pit_face_item* send_interest(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* x, struct pit_face_item* p);
void process_incoming_interest2(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, struct ccn_parsed_interest* pi, struct ccn_indexbuf* comps);
void process_incoming_content2(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, struct ccn_parsed_ContentObject* co);
}
#include "core/scheduler.h"
#include "face/facemgr.h"
namespace ndnfd {

void Strategy::Init(void) {
  // TODO move this to strategy layer master
  this->ccnd_strategy_interface_ = this->New<CcndStrategyInterface>();
}

void Strategy::OnInterest(Ptr<const InterestMessage> interest) {
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(interest->incoming_face());
  if (in_face == nullptr) return;
  in_face->CountInterestIn();

  int interest_scope = interest->parsed()->scope;
  if ((interest_scope == 0 || interest_scope == 1) && in_face->kind() != FaceKind::kApp) {
    this->Log(kLLWarn, kLCStrategy, "Strategy::OnInterest(%" PRI_FaceId ",%s) out of scope", in_face->id(), interest->name()->ToUri().c_str());
    ++CCNDH->interests_dropped;
    return;
  }
  ++CCNDH->interests_accepted;
  
  Ptr<PitEntry> ie = this->global()->npt()->GetPit(interest);
  Ptr<NamePrefixEntry> npe;
  if (ie != nullptr) {
    npe = ie->npe();
  } else {
    npe = this->global()->npt()->Seek(interest->name());
  }
  
  npe->EnsureUpdatedFib();
  if ((npe->native()->flags & CCN_FORW_LOCAL) != 0 && in_face->kind() != FaceKind::kApp) {
    this->Log(kLLWarn, kLCStrategy, "Strategy::OnInterest(%" PRI_FaceId ",%s) non local", in_face->id(), interest->name()->ToUri().c_str());
    return;
  }
  
  if (ie != nullptr) {
    this->PropagateInterest(interest, npe);
    return;
  }
  
  // TODO match ContentStore

  this->PropagateInterest(interest, npe);
}

void Strategy::PropagateInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  Ptr<PitEntry> ie = this->global()->npt()->SeekPit(interest, npe);
  bool is_new_ie = ie->native()->strategy.renewals == 0;
  Ptr<Face> inface = this->global()->facemgr()->GetFace(interest->incoming_face());
  if (inface == nullptr) return;// face is gone in another thread

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
    ie->native()->strategy.renewed = CCNDH->wtnow;
    ie->native()->strategy.renewals += 1;
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
  this->global()->scheduler()->Schedule(next_evt, std::bind(&Strategy::DoPropagate, this, ie), &ie->native()->ev, true);
}

std::unordered_set<FaceId> Strategy::LookupOutbounds(Ptr<PitEntry> ie, Ptr<const InterestMessage> interest) {
  return ie->npe()->LookupFib(interest);
}

void Strategy::PopulateOutbounds(Ptr<PitEntry> ie, const std::unordered_set<FaceId>& outbounds) {
  for (FaceId face : outbounds) {
    Ptr<PitUpstreamRecord> p = ie->SeekUpstream(face);
    if (p->IsExpired()) {
      p->SetExpiry(std::chrono::microseconds::zero());
      p->native()->pfi_flags &= ~CCND_PFI_UPHUNGRY;
    }
  }
}

void Strategy::PropagateNewInterest(Ptr<PitEntry> ie) {
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
    if ((p->native()->expiry - now) * 8 <= (p->native()->expiry - p->native()->renewed)) {// will expire soon (less than 1/8 remaining lifetime)
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
      //TODO if (npe->best_faceid() == p->faceid() || npe->prev_faceid() == p->faceid()) npe->UpdateBestFace(FaceId_none);
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
      p->native()->pfi_flags |= CCND_PFI_UPHUNGRY;
    }
  }
  
  if (debug_list.back()==',') debug_list.resize(debug_list.size()-1);
  debug_list.append("]");
#undef DEBUG_APPEND_FaceId
  this->Log(kLLDebug, kLCStrategy, "Strategy::DoPropagate(%" PRI_PitEntrySerial ") %s", ie->serial(), debug_list.c_str());

  if (upstreams == 0 && pending == 0) {
    this->WillEraseTimedOutPendingInterest(ie);
    ie->native()->ev = nullptr;
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
  upstream->set_native(send_interest(CCNDH, ie->native(), downstream->native(), upstream->native()));

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
}

void Strategy::WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillEraseTimedOutPendingInterest(%" PRI_PitEntrySerial ")", ie->serial());
}

void Strategy::OnContent(Ptr<const ContentObjectMessage> co) {
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(co->incoming_face());
  process_incoming_content2(CCNDH, in_face->ccnd_face(), static_cast<unsigned char*>(const_cast<uint8_t*>(co->msg())), co->length(), const_cast<ccn_parsed_ContentObject*>(co->parsed()));
}

void Strategy::WillSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const Message> co) {
  FaceId upstream = co->incoming_face();
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillSatisfyPendingInterest(%" PRI_PitEntrySerial ") upstream=%" PRI_FaceId "", ie->serial(), upstream);
}

void Strategy::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {
}

void Strategy::OnNack(Ptr<const NackMessage> nack) {
  this->Log(kLLWarn, kLCStrategy, "Strategy::OnNack(%s,%" PRI_FaceId ",%s) not-implemented", NackCode_string(nack->code()).c_str(), nack->incoming_face(), nack->interest()->name()->ToUri().c_str());
}

void Strategy::DidAddFibEntry(Ptr<ForwardingEntry> forw) {
  assert(forw != nullptr);
  Ptr<NamePrefixEntry> npe = forw->npe();
  FaceId faceid = forw->face();
  this->Log(kLLDebug, kLCStrategy, "Strategy::DidAddFibEntry(%s,%" PRI_FaceId ")", npe->name()->ToUri().c_str(), faceid);
}

};//namespace ndnfd
