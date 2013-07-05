#include "strategy.h"
#include "core/scheduler.h"
#include "face/facemgr.h"
extern "C" {
int match_interests(struct ccnd_handle* h, struct content_entry* content, struct ccn_parsed_ContentObject* pc, struct face* face, struct face* from_face);
}
namespace ndnfd {

void Strategy::OnInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe, Ptr<PitEntry> ie) {
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(interest->incoming_face());

  npe->EnsureUpdatedFib();
  // drop if npe is under local namespace but in_face is not local
  if ((npe->native()->flags & CCN_FORW_LOCAL) != 0 && in_face->kind() != FaceKind::kApp) {
    this->Log(kLLWarn, kLCStrategy, "Strategy::OnInterest(%" PRI_FaceId ",%s) non local", in_face->id(), interest->name()->ToUri().c_str());
    return;
  }
  
  if (ie != nullptr && ie->native()->strategy.renewals != 0) {
    this->PropagateInterest(interest, npe);
    return;
  }
  
  Ptr<ContentEntry> ce = this->global()->cs()->Lookup(interest);
  if (ce != nullptr) {
    this->SendContent(in_face->id(), ce);
    // TODO wrap match_interests
    match_interests(CCNDH, ce->native(), const_cast<ccn_parsed_ContentObject*>(ce->parsed()), in_face->ccnd_face(), nullptr);
    return;
  }

  this->PropagateInterest(interest, npe);
}

void Strategy::PropagateInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  Ptr<PitEntry> ie = this->global()->npt()->SeekPit(interest, npe);
  bool is_new_ie = ie->native()->strategy.renewals == 0;
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(interest->incoming_face());

  Ptr<PitDownstreamRecord> p = ie->SeekDownstream(in_face->id());
  // read nonce from Interest, or generate one.
  p->UpdateNonce(interest);
  // verify whether nonce is unique
  if (ie->IsNonceUnique(p)) {
    // unique nonce
    ie->native()->strategy.renewed = CCNDH->wtnow;
    ie->native()->strategy.renewals += 1;
    if (!p->pending()) {
      p->set_pending(true);
      in_face->ccnd_face()->pending_interests += 1;
    }
  } else {
    // duplicate nonce
    // record in_face as a downstream, but don't forward Interest (because lack of pending flag)
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
  this->SchedulePropagate(ie, next_evt);
}

void Strategy::SchedulePropagate(Ptr<PitEntry> ie, std::chrono::microseconds defer) {
  this->global()->scheduler()->Schedule(defer, std::bind(&Strategy::DoPropagate, this, ie), &ie->native()->ev, true);
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
    return Scheduler::kNoMore_NoCleanup;
  }
  
  return ie->NextEventDelay(false);
}

void Strategy::WillSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const Message> co, int pending_downstreams) {
  Ptr<PitUpstreamRecord> p = ie->GetUpstream(co->incoming_face());
  if (p != nullptr) {
    p->set_pending(false);
  }
}

void Strategy::WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie) {
  this->Log(kLLDebug, kLCStrategy, "Strategy::WillEraseTimedOutPendingInterest(%" PRI_PitEntrySerial ")", ie->serial());
}

void Strategy::OnContent(Ptr<const ContentObjectMessage> co) {
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
