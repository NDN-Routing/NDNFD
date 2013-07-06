#include "layer.h"
#include <stack>
#include "core/scheduler.h"
#include "face/facemgr.h"
extern "C" {
uint32_t WTHZ_value(void);
int match_interests(struct ccnd_handle* h, struct content_entry* content, struct ccn_parsed_ContentObject* pc, struct face* face, struct face* from_face);
}

// TODO remove these after impl dispatch
#include "strategy/original.h"
#include "strategy/selflearn.h"
#define STRATEGY_TYPE OriginalStrategy

namespace ndnfd {

void StrategyLayer::Init(void) {
  this->ccnd_strategy_interface_ = this->New<CcndStrategyInterface>();
  // TODO set strategy on "ccnx:/"
}

Ptr<Strategy> StrategyLayer::GetStrategy(StrategyType t) {
  Ptr<Strategy> s = this->strategy_arr_.at(t);
  if (s == nullptr) {
    StrategyCtor ctor = std::get<1>(StrategyType_list().at(t));
    this->strategy_arr_[t] = s = ctor(this);
  }
  return s;
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
  return this->GetStrategy(t);
}

void StrategyLayer::SetStrategy(Ptr<const Name> prefix, StrategyType t) {
  Ptr<NamePrefixEntry> npe = this->global()->npt()->Seek(prefix);
  npe->set_strategy_type(t);
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
  Ptr<Face> in_face = this->global()->facemgr()->GetFace(co->incoming_face());
  if (in_face == nullptr) return;
  in_face->CountContentObjectIn();
  
  ContentStore::AddResult res; Ptr<ContentEntry> ce;
  std::tie(res, ce) = this->global()->cs()->Add(co);
  
  if (res == ContentStore::AddResult::New || res == ContentStore::AddResult::Refreshed) {
    this->ccnd_strategy_interface()->ce_ = ce;
    this->ccnd_strategy_interface()->co_ = co;
    int n_matches = match_interests(CCNDH, ce->native(), const_cast<ccn_parsed_ContentObject*>(co->parsed()), nullptr, in_face->native());
    this->ccnd_strategy_interface()->ce_ = nullptr;
    this->ccnd_strategy_interface()->co_ = nullptr;
    if (res == ContentStore::AddResult::New) {
      if (n_matches < 0) {
        this->global()->cs()->Remove(ce);
      }
      if (n_matches == 0) {
        if (in_face->kind() != FaceKind::kApp && in_face->kind() != FaceKind::kInternal) {
          ce->set_unsolicited(true);
        }
        this->DidReceiveUnsolicitedContent(ce, co);
      }
    }
  }
}

void StrategyLayer::DidSatisfyPendingInterestInternal(Ptr<PitEntry> ie, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int pending_downstreams) {
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
  // unset pending on PitUpstreamRecord
  Ptr<PitUpstreamRecord> p = ie->GetUpstream(co->incoming_face());
  if (p != nullptr) {
    p->set_pending(false);
  }

  Ptr<Strategy> strategy = this->FindStrategy(ie->npe());
  strategy->DidSatisfyPendingInterest(ie, ce, co, pending_downstreams);
}

void StrategyLayer::DidReceiveContentInternal(Ptr<NamePrefixEntry> npe, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int matching_suffix) {
  Ptr<Strategy> strategy = this->FindStrategy(npe);
  strategy->DidReceiveContent(npe, ce, co, matching_suffix);
}

void StrategyLayer::DidReceiveUnsolicitedContent(Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co) {
  Ptr<Strategy> strategy = this->FindStrategy(ce->name());
  strategy->DidReceiveUnsolicitedContent(ce, co);
}

void StrategyLayer::OnNack(Ptr<const NackMessage> nack) {
  // TODO impl common procedures & dispatch
  Ptr<Strategy> strategy = this->FindStrategy(nack->interest()->name());
  strategy->OnNack(nack);
}

void StrategyLayer::NewNpeExtra(Ptr<NamePrefixEntry> npe) {
  this->Log(kLLDebug, kLCStrategy, "StrategyLayer::NewNpeExtra(%s)", npe->name()->ToUri().c_str());
  Ptr<Strategy> strategy = this->FindStrategy(npe);
  npe->set_strategy_extra_type(strategy->strategy_type());
  Ptr<NamePrefixEntry> parent = npe->Parent();
  if (parent != nullptr && this->FindStrategy(parent) == strategy) {
    strategy->InheritNpeExtra(npe, parent);
  } else {
    strategy->NewNpeExtra(npe);
  }
}

void StrategyLayer::FinalizeNpeExtra(Ptr<NamePrefixEntry> npe) {
  Ptr<Strategy> strategy = this->FindStrategy(npe);
  strategy->FinalizeNpeExtra(npe);
}

void StrategyLayer::UpdateNpeExtra(Ptr<NamePrefixEntry> npe) {
  this->Log(kLLDebug, kLCStrategy, "StrategyLayer::UpdateNpeExtra(%s)", npe->name()->ToUri().c_str());
  Ptr<NamePrefixEntry> root = npe->StrategyNode();
  assert(root != nullptr);
  std::stack<Ptr<NamePrefixEntry>> stack;
  for (; ; npe = npe->Parent()) {
    StrategyType old_type = npe->strategy_extra_type();
    if (old_type == root->strategy_type()) break;
    if (old_type != StrategyType_none) {
      Ptr<Strategy> old_strategy = this->GetStrategy(old_type);
      old_strategy->FinalizeNpeExtra(npe);
    }
    stack.push(npe);
    if (npe == root) break;
  }
  Ptr<Strategy> strategy = this->GetStrategy(root->strategy_type());
  while (!stack.empty()) {
    auto n = stack.top();
    n->set_strategy_extra_type(root->strategy_type());
    if (n == root) {
      strategy->NewNpeExtra(n);
    } else {
      strategy->InheritNpeExtra(n, n->Parent());
    }
    stack.pop();
  }
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
