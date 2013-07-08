#include "nacks.h"
#include "face/facemgr.h"
namespace ndnfd {

StrategyType_def(NacksStrategy, nacks);

std::string NacksStrategy::FaceStatus_string(FaceStatus value) {
  switch (value) {
    case FaceStatus::kGreen : return "GREEN";
    case FaceStatus::kYellow: return "YELLOW";
    case FaceStatus::kRed   : return "RED";
  }
  return "";
}

void NacksStrategy::PropagateInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  Ptr<PitEntry> ie = this->global()->npt()->SeekPit(interest, npe);
  bool is_new_ie = ie->native()->strategy.renewals == 0;
  this->global()->scheduler()->Cancel(ie->native()->ev);
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
      in_face->native()->pending_interests += 1;
    }
  } else {
    // duplicate nonce
    // record in_face as a downstream, but don't forward Interest (because lack of pending flag)
    p->set_suppress(true);
    this->SendNack(ie, p, NackCode::kDuplicate);
    return;
  }
  
  // set expiry time according to InterestLifetime
  p->SetExpiryToLifetime(interest);
  
  // lookup FIB and populate PitUpstreamRecords
  std::unordered_set<FaceId> outbounds = this->LookupOutbounds(ie, interest);
  this->PopulateOutbounds(ie, outbounds);
  
  // forward
  if (is_new_ie || this->IsRetryTimerExpired(ie)) {
    this->Forward(ie);
  }
}

void NacksStrategy::Forward(Ptr<PitEntry> ie) {
  if (ie->beginUpstream() == ie->endUpstream()) {
    this->SendNacks(ie, NackCode::kNoData);
    return;
  }
  bool propagated = this->DoForward(ie);
  if (!propagated) {
    this->SendNacks(ie, NackCode::kCongestion);
    return;
  }
}

bool NacksStrategy::DoForward(Ptr<PitEntry> ie) {
  NpeExtra* extra = ie->npe()->GetStrategyExtra<NpeExtra>();
  
  auto try_upstreams_with_status = [&] (FaceStatus expect_status) ->bool {
    for (auto it = ie->beginUpstream(); it != ie->endUpstream(); ++it) {
      Ptr<PitUpstreamRecord> p = *it;
      if (p->GetFlag(NacksStrategy::PFI_VAIN)) continue;
      if (extra->status_[p->faceid()] == expect_status) {
        if (this->SendInterest(ie, p)) {
          this->Log(kLLDebug, kLCStrategy, "NacksStrategy::DoForward(%" PRI_PitEntrySerial ") %s %" PRI_FaceId "", ie->serial(), FaceStatus_string(expect_status).c_str(), p->faceid());
          this->SetRetryTimer(ie, std::chrono::microseconds(1000000));
          return true;
        }
      }
    }
    return false;
  };
  
  if (try_upstreams_with_status(FaceStatus::kGreen)) return true;
  if (try_upstreams_with_status(FaceStatus::kYellow)) return true;
  return false;
}

void NacksStrategy::DidSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int pending_downstreams) {
  this->global()->scheduler()->Cancel(ie->native()->strategy.ev);
  Ptr<NamePrefixEntry> npe = this->GetFaceStatusNode(ie->npe());
  NpeExtra* extra = npe->GetStrategyExtra<NpeExtra>();
  extra->status_[co->incoming_face()] = FaceStatus::kGreen;
}

void NacksStrategy::OnNack(Ptr<const NackMessage> nack) {
  Ptr<PitEntry> ie = this->global()->npt()->GetPit(nack->interest());
  if (ie == nullptr) return;
  if (this->IsRetryTimerExpired(ie)) return;
  InterestMessage::Nonce nonce = nack->interest()->nonce();
  if (std::none_of(ie->beginUpstream(), ie->endUpstream(), std::bind(&PitUpstreamRecord::NonceEquals, std::placeholders::_1, nonce))) return;
  this->Forward(ie);
}

Ptr<NamePrefixEntry> NacksStrategy::GetFaceStatusNode(Ptr<NamePrefixEntry> npe) {
  Ptr<NamePrefixEntry> n = npe->FibNode();
  if (n == nullptr) {
    n = this->global()->npt()->Seek(Name::FromUri("/"));
  }
  return n;
}

void NacksStrategy::SetRetryTimer(Ptr<PitEntry> ie, std::chrono::microseconds delay) {
  ie->native()->strategy.state &= ~NacksStrategy::IE_RETRY_TIMER_EXPIRED;
  this->global()->scheduler()->Schedule(delay, [&ie](){
    ie->native()->strategy.state |= NacksStrategy::IE_RETRY_TIMER_EXPIRED;
    return Scheduler::kNoMore;
  }, &ie->native()->strategy.ev, true);
}

bool NacksStrategy::IsRetryTimerExpired(Ptr<PitEntry> ie) {
  return (ie->native()->strategy.state & NacksStrategy::IE_RETRY_TIMER_EXPIRED) != 0;
}

void NacksStrategy::NewNpeExtra(Ptr<NamePrefixEntry> npe) {
  npe->set_strategy_extra(new NpeExtra());
}

void NacksStrategy::InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent) {
  NpeExtra* parent_extra = parent->GetStrategyExtra<NpeExtra>();
  npe->set_strategy_extra(new NpeExtra(*parent_extra));
}

void NacksStrategy::FinalizeNpeExtra(Ptr<NamePrefixEntry> npe) {
  NpeExtra* extra = npe->detach_strategy_extra<NpeExtra>();
  delete extra;
}

};//namespace ndnfd
