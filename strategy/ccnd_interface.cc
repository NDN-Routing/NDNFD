#include "ccnd_interface.h"
#include "strategy/layer.h"
#include "face/facemgr.h"
extern "C" {
void strategy_callout2_SATISFIED(struct ccnd_handle* h, struct interest_entry* ie, int pending_downstreams) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->sl()->ccnd_strategy_interface()->DidSatisfyPendingInterest(ie, pending_downstreams);
}

void note_content_from2(struct ccnd_handle* h, struct nameprefix_entry* npe, int matching_suffix) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->sl()->ccnd_strategy_interface()->DidReceiveContent(npe, matching_suffix);
}

void update_npe_children(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->sl()->ccnd_strategy_interface()->DidAddFibEntry(npe, static_cast<ndnfd::FaceId>(faceid));
}
}
namespace ndnfd {

void CcndStrategyInterface::DidSatisfyPendingInterest(interest_entry* ie, int pending_downstreams) {
  if (this->ce_ == nullptr) return;
  Ptr<PitEntry> ie1 = this->New<PitEntry>(ie);
  this->global()->sl()->DidSatisfyPendingInterestInternal(ie1, this->ce_, this->co_, pending_downstreams);
}

void CcndStrategyInterface::DidReceiveContent(nameprefix_entry* npe, int matching_suffix) {
  if (this->ce_ == nullptr) return;
  Ptr<NamePrefixEntry> npe1 = static_cast<NamePrefixEntry*>(npe->ndnfd_npe);
  this->global()->sl()->DidReceiveContentInternal(npe1, this->ce_, this->co_, matching_suffix);
}

void CcndStrategyInterface::DidAddFibEntry(nameprefix_entry* npe, FaceId faceid) {
  Ptr<NamePrefixEntry> npe1 = static_cast<NamePrefixEntry*>(npe->ndnfd_npe);
  Ptr<ForwardingEntry> forw = npe1->GetForwarding(faceid);
  assert(forw != nullptr);
  this->global()->sl()->DidAddFibEntry(forw);
}

};//namespace ndnfd

