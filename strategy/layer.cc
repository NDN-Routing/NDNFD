#include "layer.h"
#include "core/scheduler.h"
#include "face/facemgr.h"
extern "C" {
void process_incoming_content2(struct ccnd_handle* h, struct face* face, unsigned char* msg, size_t size, struct ccn_parsed_ContentObject* co);
uint32_t WTHZ_value(void);
}

// TODO remove these after impl dispatch
#include "strategy/original.h"
#define STRATEGY_TYPE OriginalStrategy

namespace ndnfd {

void StrategyLayer::Init(void) {
  this->ccnd_strategy_interface_ = this->New<CcndStrategyInterface>();
  // TODO set strategy on "ccnx:/"
}

Ptr<Strategy> StrategyLayer::FindStrategy(Ptr<const Name> name) {
  for (Ptr<const Name> n = name; n != nullptr; n = n->StripSuffix(1)) {
    Ptr<NamePrefixEntry> npe = this->global()->npt()->Get(n);
    if (npe != nullptr) return this->FindStrategy(npe);
  }
  return nullptr;
}

Ptr<Strategy> StrategyLayer::FindStrategy(Ptr<const NamePrefixEntry> npe) {
  Ptr<NamePrefixEntry> root = npe->StrategyNode();
  if (root == nullptr) {
    // set default strategy type
    Ptr<Name> root_name = Name::FromUri("/");
    root = this->global()->npt()->Seek(root_name);
    root->set_strategy_type(STRATEGY_TYPE::kType);
    root = npe->StrategyNode();
  }
  assert(root != nullptr);
  StrategyType t = root->strategy_type();
  Ptr<Strategy> strategy = this->strategy_arr_.at(t);
  if (strategy == nullptr) {
    StrategyCtor ctor = std::get<1>(StrategyType_list().at(t));
    this->strategy_arr_[t] = strategy = ctor(this);
  }
  return strategy;
}

void StrategyLayer::OnInterest(Ptr<const InterestMessage> interest) {
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(interest->incoming_face());
  if (in_face == nullptr) return;
  in_face->CountInterestIn();

  int interest_scope = interest->parsed()->scope;
  if ((interest_scope == 0 || interest_scope == 1) && in_face->kind() != FaceKind::kApp) {
    this->Log(kLLWarn, kLCStrategy, "StrategyLayer::OnInterest(%" PRI_FaceId ",%s) out of scope", in_face->id(), interest->name()->ToUri().c_str());
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
  
  Ptr<Strategy> strategy = this->FindStrategy(npe);
  strategy->OnInterest(interest, npe, ie);
}

void StrategyLayer::OnContent(Ptr<const ContentObjectMessage> co) {
  // TODO impl common procedures & dispatch
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(co->incoming_face());
  process_incoming_content2(CCNDH, in_face->ccnd_face(), static_cast<unsigned char*>(const_cast<uint8_t*>(co->msg())), co->length(), const_cast<ccn_parsed_ContentObject*>(co->parsed()));
}

void StrategyLayer::WillSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const Message> co, int pending_downstreams) {
  if (pending_downstreams > 0) {
    // mark PitEntry as consumed, so that new Interest is considered new
    ie->native()->strategy.renewals = 0;
    
    // schedule delete PitEntry on last upstream expiry
    ccn_wrappedtime now = CCNDH->wtnow;
    ccn_wrappedtime last = 0;
    std::for_each(ie->beginUpstream(), ie->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
      if (!p->IsExpired()) {
        last = std::max(last, p->native()->expiry - now);
      }
    });
    this->global()->scheduler()->Schedule(std::chrono::microseconds(last * (1000000 / WTHZ_value())), [this,ie]{
      this->global()->npt()->DeletePit(ie);
      return Scheduler::kNoMore_NoCleanup;
    }, &ie->native()->ev, true);
  }
  
  Ptr<Strategy> strategy = this->FindStrategy(ie->npe());
  strategy->WillSatisfyPendingInterest(ie, co, pending_downstreams);
}

void StrategyLayer::DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {
}

void StrategyLayer::OnNack(Ptr<const NackMessage> nack) {
  // TODO impl common procedures & dispatch
  Ptr<Strategy> strategy = this->FindStrategy(nack->interest()->name());
  strategy->OnNack(nack);
}

void StrategyLayer::NewNpeExtra(Ptr<NamePrefixEntry> npe) {
  Ptr<Strategy> strategy = this->FindStrategy(npe);
  strategy->NewNpeExtra(npe);
}

void StrategyLayer::InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent) {
  Ptr<Strategy> strategy = this->FindStrategy(npe);
  Ptr<Strategy> strategy_parent = this->FindStrategy(parent);
  if (strategy == strategy_parent) {
    strategy->InheritNpeExtra(npe, parent);
  } else {
    strategy->NewNpeExtra(npe);
  }
}

void StrategyLayer::FinalizeNpeExtra(Ptr<NamePrefixEntry> npe) {
  Ptr<Strategy> strategy = this->FindStrategy(npe);
  strategy->FinalizeNpeExtra(npe);
}

void StrategyLayer::DidAddFibEntry(Ptr<ForwardingEntry> forw) {
  Ptr<Strategy> strategy = this->FindStrategy(forw->npe());
  strategy->DidAddFibEntry(forw);
}

void StrategyLayer::WillRemoveFibEntry(Ptr<ForwardingEntry> forw) {
  Ptr<Strategy> strategy = this->FindStrategy(forw->npe());
  strategy->WillRemoveFibEntry(forw);
}

};//namespace ndnfd
