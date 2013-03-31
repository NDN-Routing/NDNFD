#include "ccnd_interface.h"
#include "strategy.h"
using ndnfd::Global;

int propagate_interest(struct ccnd_handle* h, struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->PropagateInterest(face, msg, pi, npe);
}

void strategy_callout2_SATISFIED(struct ccnd_handle* h, struct interest_entry* ie, struct face* from_face) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->WillSatisfyPendingInterest(ie, static_cast<ndnfd::FaceId>(from_face->faceid));
}

void update_npe_children2(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid, const uint8_t* name, size_t name_size) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->DidAddFibEntry(npe, static_cast<ndnfd::FaceId>(faceid), name, name_size);
}

namespace ndnfd {

int CcndStrategyInterface::PropagateInterest(face* face, uint8_t* msg, ccn_parsed_interest* pi, nameprefix_entry* npe) {
  Ptr<InterestMessage> interest = new InterestMessage(msg, pi->offset[CCN_PI_E], pi);
  interest->set_incoming_face(static_cast<FaceId>(face->faceid));
  Ptr<NamePrefixEntry> npe1 = this->New<NamePrefixEntry>(interest->name(), npe);
  this->global()->strategy()->PropagateInterest(interest, npe1);
  return 0;
}

void CcndStrategyInterface::WillSatisfyPendingInterest(interest_entry* ie, FaceId upstream) {
  Ptr<PitEntry> ie1 = this->New<PitEntry>(ie);
  this->global()->strategy()->WillSatisfyPendingInterest(ie1, upstream);
}

void CcndStrategyInterface::DidAddFibEntry(nameprefix_entry* npe, FaceId faceid, const uint8_t* name, size_t name_size) {
  Ptr<Name> name1 = Name::FromCcnb(name, name_size);
  Ptr<NamePrefixEntry> npe1 = this->New<NamePrefixEntry>(name1, npe);
  Ptr<ForwardingEntry> forw = npe1->GetForwarding(faceid);
  assert(forw != nullptr);
  this->global()->strategy()->DidAddFibEntry(forw);
}

};//namespace ndnfd

