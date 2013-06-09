#include "ccnd_interface.h"
#include "strategy.h"
#include "face/facemgr.h"
using ndnfd::Global;

int propagate_interest(struct ccnd_handle* h, struct face* face, uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->PropagateInterest(face, msg, pi, npe);
}

void strategy_callout2_SATISFIED(struct ccnd_handle* h, struct interest_entry* ie, struct face* from_face) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->WillSatisfyPendingInterest(ie, static_cast<ndnfd::FaceId>(from_face->faceid));
}

void note_content_from2(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned from_faceid, const uint8_t* name, size_t name_size, int matching_suffix) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->DidSatisfyPendingInterests(npe, static_cast<ndnfd::FaceId>(from_faceid), ndnfd::Name::FromCcnb(name, name_size), matching_suffix);
}

void update_npe_children2(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid, const uint8_t* name, size_t name_size) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->DidAddFibEntry(npe, static_cast<ndnfd::FaceId>(faceid), ndnfd::Name::FromCcnb(name, name_size));
}

void ndnfd_npe_strategy_extra_create(struct ccnd_handle* h, struct nameprefix_entry* npe, const uint8_t* name, size_t name_size) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->CreateNpe(npe, ndnfd::Name::FromCcnb(name, name_size));
}

void ndnfd_npe_strategy_extra_finalize(struct ccnd_handle* h, struct nameprefix_entry* npe, const uint8_t* name, size_t name_size) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  return global->strategy()->ccnd_strategy_interface()->FinalizeNpe(npe, ndnfd::Name::FromCcnb(name, name_size));
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
  Ptr<Message> co = this->global()->facemgr()->ccnd_face_interface()->last_received_message_;
  // TODO co becomes Ptr<const ContentObjectMessage> once buf decoder is ready
  // TODO test for co->type() == ContentObjectMessage::kType and cast to Ptr<const ContentObjectMessage>
  // don't assert here: message from internal client is not placed on CcndFaceInterface::last_received_message_
  if (co == nullptr || co->incoming_face() != upstream) {
    this->Log(kLLDebug, kLCStrategy, "CcndStrategyInterface::WillSatisfyPendingInterest(%" PRI_FaceId ")", upstream);
    return;
  }

  Ptr<PitEntry> ie1 = this->New<PitEntry>(ie);
  this->global()->strategy()->WillSatisfyPendingInterest(ie1, co);
}

void CcndStrategyInterface::DidSatisfyPendingInterests(nameprefix_entry* npe, FaceId upstream, Ptr<Name> name, int matching_suffix) {
  Ptr<Message> co = this->global()->facemgr()->ccnd_face_interface()->last_received_message_;
  // TODO co becomes Ptr<const ContentObjectMessage> once buf decoder is ready
  // TODO test for co->type() == ContentObjectMessage::kType and cast to Ptr<const ContentObjectMessage>
  // don't assert here: message from internal client is not placed on CcndFaceInterface::last_received_message_
  if (co == nullptr || co->incoming_face() != upstream) {
    this->Log(kLLDebug, kLCStrategy, "CcndStrategyInterface::DidSatisfyPendingInterests(%" PRI_FaceId ",%s)", upstream, name->ToUri().c_str());
    return;
  }

  Ptr<NamePrefixEntry> npe1 = this->New<NamePrefixEntry>(name, npe);
  this->global()->strategy()->DidSatisfyPendingInterests(npe1, co, matching_suffix);
}

void CcndStrategyInterface::DidAddFibEntry(nameprefix_entry* npe, FaceId faceid, Ptr<Name> name) {
  Ptr<NamePrefixEntry> npe1 = this->New<NamePrefixEntry>(name, npe);
  Ptr<ForwardingEntry> forw = npe1->GetForwarding(faceid);
  assert(forw != nullptr);
  this->global()->strategy()->DidAddFibEntry(forw);
}

void CcndStrategyInterface::CreateNpe(nameprefix_entry* npe, Ptr<Name> name) {
  Ptr<NamePrefixEntry> npe1 = this->New<NamePrefixEntry>(name, npe);
  Ptr<NamePrefixEntry> parent = npe1->Parent();
  if (parent == nullptr) this->global()->strategy()->NewNpeExtra(npe1);
  else this->global()->strategy()->InheritNpeExtra(npe1, parent);
  // TODO invoke NewNpeExtra if child namespace is using a different strategy
}

void CcndStrategyInterface::FinalizeNpe(nameprefix_entry* npe, Ptr<Name> name) {
  Ptr<NamePrefixEntry> npe1 = this->New<NamePrefixEntry>(name, npe);
  this->global()->strategy()->FinalizeNpeExtra(npe1);
}

};//namespace ndnfd

