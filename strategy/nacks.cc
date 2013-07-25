#include "nacks.h"
#include "face/facemgr.h"
namespace ndnfd {

StrategyType_def(NacksStrategy, nacks);

std::string NacksStrategy::UpstreamStatus_string(UpstreamStatus value) {
  switch (value) {
    case UpstreamStatus::kGreen : return "GREEN";
    case UpstreamStatus::kYellow: return "YELLOW";
    case UpstreamStatus::kRed   : return "RED";
  }
  return "";
}

void NacksStrategy::PropagateInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  Ptr<PitEntry> ie = this->global()->npt()->SeekPit(interest, npe);
  bool is_new_ie = ie->IsNew();
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(interest->incoming_face());

  Ptr<PitDownstreamRecord> p = ie->SeekDownstream(in_face->id());
  // read nonce from Interest, or generate one.
  p->UpdateNonce(interest);
  // verify whether nonce is unique
  if (ie->IsNonceUnique(p)) {
    // unique nonce
    ie->Renew();
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
  NpeExtra* extra = this->GetUpstreamStatusNode(ie->npe())->GetStrategyExtra<NpeExtra>();
  
  auto try_upstreams_with_status = [&] (UpstreamStatus expect_status) ->bool {
    for (auto it = ie->beginUpstream(); it != ie->endUpstream(); ++it) {
      Ptr<PitUpstreamRecord> p = *it;
      if (p->GetFlag(NacksStrategy::PFI_VAIN)) continue;
      UpstreamExtra& ue = extra->table_[p->faceid()];
      if (ue.status_ == expect_status) {
        std::chrono::microseconds rtt = ue.rtt_.RetransmitTimeout();
        if (this->SendInterest(ie, p)) {
          ie->RttStartWithExpect(p->faceid(), rtt, [&ue](){
            if (ue.status_ == UpstreamStatus::kGreen) ue.status_ = UpstreamStatus::kYellow;
          });
          this->SetRetryTimer(ie, rtt);
          this->Log(kLLDebug, kLCStrategy, "NacksStrategy::DoForward(%" PRI_PitEntrySerial ") %s %" PRI_FaceId " retry=%" PRIuMAX, ie->serial(), UpstreamStatus_string(expect_status).c_str(), p->faceid(), static_cast<uintmax_t>(rtt.count()));
          return true;
        }
      }
    }
    return false;
  };
  
  if (try_upstreams_with_status(UpstreamStatus::kGreen)) return true;
  if (try_upstreams_with_status(UpstreamStatus::kYellow)) return true;
  // TODO mark YELLOW on Interest timeout
  return false;
}

void NacksStrategy::DidSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int pending_downstreams) {
  this->global()->scheduler()->Cancel(ie->native()->strategy.ev);
  NpeExtra* extra = this->GetUpstreamStatusNode(ie->npe())->GetStrategyExtra<NpeExtra>();
  UpstreamExtra& ue = extra->table_[co->incoming_face()];
  ue.status_ = UpstreamStatus::kGreen;
  std::chrono::microseconds rtt = ie->RttEnd(co->incoming_face());
  if (rtt.count() >= 0) ue.rtt_.Measurement(rtt);
}

void NacksStrategy::OnNack(Ptr<const NackMessage> nack) {
  Ptr<PitEntry> ie = this->global()->npt()->GetPit(nack->interest());
  if (ie == nullptr) return;
  ie->RttEnd(nack->incoming_face());// don't set to yellow
  if (this->IsRetryTimerExpired(ie)) return;
  InterestMessage::Nonce nonce = nack->interest()->nonce();
  if (std::none_of(ie->beginUpstream(), ie->endUpstream(), std::bind(&PitUpstreamRecord::NonceEquals, std::placeholders::_1, nonce))) return;
  this->Forward(ie);
}

Ptr<NamePrefixEntry> NacksStrategy::GetUpstreamStatusNode(Ptr<NamePrefixEntry> npe) {
  Ptr<NamePrefixEntry> n = npe->FibNode();
  if (n == nullptr) {
    n = this->global()->npt()->Seek(Name::FromUri("/"));
  }
  return n;
}

void NacksStrategy::SetRetryTimer(Ptr<PitEntry> ie, std::chrono::microseconds delay) {
  ie->native()->strategy.state &= ~NacksStrategy::IE_RETRY_TIMER_EXPIRED;
  this->global()->scheduler()->Schedule(delay, [ie](){
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
