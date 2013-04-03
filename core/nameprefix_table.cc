#include "nameprefix_table.h"
#include <algorithm>
#include <limits>
extern "C" {
#include <ccn/hashtb.h>
uint32_t WTHZ_value(void);
int nameprefix_seek(struct ccnd_handle* h, struct hashtb_enumerator* e, const uint8_t* msg, struct ccn_indexbuf* comps, int ncomps);
struct ccn_forwarding* seek_forwarding(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid);
void update_forward_to(struct ccnd_handle* h, struct nameprefix_entry* npe);
struct ccn_indexbuf* get_outbound_faces(struct ccnd_handle *h, struct face* from, const uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe);
void link_interest_entry_to_nameprefix(struct ccnd_handle *h, struct interest_entry *ie, struct nameprefix_entry *npe);
void consume_interest(struct ccnd_handle* h, struct interest_entry* ie);
struct pit_face_item* pfi_seek(struct ccnd_handle* h, struct interest_entry* ie, unsigned faceid, unsigned pfi_flag);
void pfi_destroy(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p);
int ie_next_usec(struct ccnd_handle* h, struct interest_entry* ie, ccn_wrappedtime* expiry);
}
#include "core/scheduler.h"
#include "face/facemgr.h"
#include "message/interest.h"
extern "C" {

void ndnfd_finalize_interest(struct interest_entry* ie) {
  ndnfd::InterestMessage* interest = ie_ndnfdInterest(ie);
  interest->Unref();
}

const struct ccn_parsed_interest* ndnfd_ie_pi(const struct interest_entry* ie) {
  ndnfd::InterestMessage* interest = ie_ndnfdInterest(ie);
  return interest->parsed();
}

}
namespace ndnfd {

Ptr<NamePrefixEntry> NamePrefixTable::SeekInternal(Ptr<const Name> name, bool create) {
  ccn_indexbuf* comps = ccn_indexbuf_create();
  std::basic_string<uint8_t> name_ccnb = name->ToCcnb(false, comps);

  nameprefix_entry* npe;
  if (create) {
    hashtb_enumerator ee; hashtb_enumerator* e = &ee;
    hashtb_start(CCNDH->nameprefix_tab, e);
    nameprefix_seek(CCNDH, e, name_ccnb.data(), comps, name->n_comps());
    npe = reinterpret_cast<nameprefix_entry*>(e->data);
    hashtb_end(e);
  } else {
    npe = reinterpret_cast<nameprefix_entry*>(hashtb_lookup(CCNDH->nameprefix_tab, name_ccnb.data(), name_ccnb.size()));
  }

  ccn_indexbuf_destroy(&comps);
  if (npe == nullptr) return nullptr;
  return this->New<NamePrefixEntry>(name, npe);
}

Ptr<PitEntry> NamePrefixTable::GetPit(Ptr<const InterestMessage> interest) {
  interest_entry* ie = reinterpret_cast<interest_entry*>(hashtb_lookup(CCNDH->interest_tab, interest->msg(), interest->parsed()->offset[CCN_PI_B_InterestLifetime]));
  if (ie == nullptr) return nullptr;
  return this->New<PitEntry>(ie);
}

Ptr<PitEntry> NamePrefixTable::SeekPit(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  assert(interest != nullptr);
  assert(npe != nullptr);
  assert(interest->name()->Equals(npe->name()));
  hashtb_enumerator ee; hashtb_enumerator* e = &ee;
  hashtb_start(CCNDH->interest_tab, e);

  int res = hashtb_seek(e, interest->msg(), interest->parsed()->offset[CCN_PI_B_InterestLifetime], 1);
  interest_entry* ie; ie = reinterpret_cast<interest_entry*>(e->data);
  if (res == HT_NEW_ENTRY) {
    ie->serial = ++CCNDH->iserial;
    ie->strategy.birth = ie->strategy.renewed = CCNDH->wtnow;
    ie->strategy.renewals = 0;
    this->Log(kLLDebug, kLCStrategy, "NamePrefixTable::SeekPit(%s) new PitEntry(%" PRIu32 ")", npe->name()->ToUri().c_str(), static_cast<uint32_t>(ie->serial));
  }
  if (ie->interest_msg == nullptr) {
    link_interest_entry_to_nameprefix(CCNDH, ie, npe->npe());
    ie->interest_msg = reinterpret_cast<const uint8_t*>(e->key);
    ie->size = interest->parsed()->offset[CCN_PI_B_InterestLifetime] + 1;
    const_cast<uint8_t*>(ie->interest_msg) [ie->size-1] = '\0';//set last byte to </Interest>
    Ptr<InterestMessage> interest2 = InterestMessage::Parse(ie->interest_msg, ie->size);
    assert(interest2 != nullptr);
    interest2->set_incoming_face(interest->incoming_face());
    interest2->set_incoming_sender(interest->incoming_sender());
    ie->ndnfd_interest = GetPointer(interest2);
  }

  hashtb_end(e);
  if (ie == nullptr) return nullptr;
  return this->New<PitEntry>(ie);
}

void NamePrefixTable::DeletePit(Ptr<PitEntry> ie) {
  assert(ie != nullptr);
  consume_interest(CCNDH, ie->ie());
}

NamePrefixEntry::NamePrefixEntry(Ptr<const Name> name, nameprefix_entry* npe) : name_(name), npe_(npe) {
  assert(name != nullptr);
  assert(npe != nullptr);
}

Ptr<NamePrefixEntry> NamePrefixEntry::Parent(void) const {
  nameprefix_entry* parent = this->npe()->parent;
  if (parent == nullptr) return nullptr;
  return this->New<NamePrefixEntry>(this->name()->StripSuffix(1), parent);
}
  
Ptr<NamePrefixEntry> NamePrefixEntry::FibNode(void) const {
  for (Ptr<NamePrefixEntry> n = const_cast<NamePrefixEntry*>(this); n != nullptr; n = n->Parent()) {
    if (n->npe()->forwarding != nullptr) return n;
  }
  return nullptr;
}

void NamePrefixEntry::EnsureUpdatedFib(void) const {
  update_forward_to(CCNDH, this->npe());
}

std::unordered_set<FaceId> NamePrefixEntry::LookupFib(Ptr<const InterestMessage> interest) const {
  assert(interest != nullptr);
  Ptr<Face> inface = this->global()->facemgr()->GetFace(interest->incoming_face());

  ccn_indexbuf* outbound = get_outbound_faces(CCNDH, inface==nullptr ? nullptr : inface->ccnd_face(), interest->msg(), const_cast<ccn_parsed_interest*>(interest->parsed()), this->npe());

  std::unordered_set<FaceId> s;
  for (size_t i = 0; i < outbound->n; ++i) {
    s.insert(outbound->buf[i]);
  }
  ccn_indexbuf_destroy(&outbound);
  return s;
}

Ptr<ForwardingEntry> NamePrefixEntry::SeekForwardingInternal(FaceId faceid, bool create) {
  ccn_forwarding* f;
  
  if (create) {
    f = seek_forwarding(CCNDH, this->npe(), static_cast<unsigned>(faceid));
    ++(CCNDH->forward_to_gen);
  } else {
    for (f = this->npe()->forwarding; f != nullptr; f = f->next) {
      if (static_cast<FaceId>(f->faceid) == faceid) break;
    }
  }
  
  if (f == nullptr) return nullptr;
  return this->New<ForwardingEntry>(this, f);
}

void NamePrefixEntry::ForeachPit(std::function<ForeachAction(Ptr<PitEntry>)> f) {
  // ForeachPit cannot be replaced with STL iterator due to the use of hashtb_enumerator.
  hashtb_enumerator ee; hashtb_enumerator* e = &ee;
  hashtb_start(CCNDH->interest_tab, e);
  for (interest_entry* ie = static_cast<interest_entry*>(e->data); ie != nullptr; ie = static_cast<interest_entry*>(e->data)) {
    ForeachAction act = ForeachAction::kNone;
    for (nameprefix_entry* x = ie->ll.npe; x != nullptr; x = x->parent) {
      if (x == this->npe()) {
        Ptr<PitEntry> ie1 = this->New<PitEntry>(ie);
        act = f(ie1);
        break;
      }
    }
    if (ForeachAction_break(act)) {
      break;
    }
    hashtb_next(e);
  }
  hashtb_end(e);
}

FaceId NamePrefixEntry::GetBestFace(void) {
  FaceId best = this->best_faceid();
  if (best == FaceId_none) {
    best = this->prev_faceid();
    this->set_best_faceid(best);
  }
  return best;
}

void NamePrefixEntry::UpdateBestFace(FaceId value) {
  if (this->best_faceid() == value) {
    adjust_npe_predicted_response(CCNDH, this->npe(), 0);
  } else if (this->best_faceid() == FaceId_none) {
    this->set_best_faceid(value);
  } else {
    this->set_prev_faceid(this->best_faceid());
    this->set_best_faceid(value);
  }
}

void NamePrefixEntry::AdjustPredictUp(void) {
  adjust_npe_predicted_response(CCNDH, this->npe(), 1);
}

ForwardingEntry::ForwardingEntry(Ptr<NamePrefixEntry> npe, ccn_forwarding* forw) : npe_(npe), forw_(forw) {
  assert(npe != nullptr);
  assert(forw != nullptr);
}

void ForwardingEntry::Refresh(std::chrono::seconds expires) {
  if (expires < std::chrono::seconds::zero()) {
    this->forw()->flags &= ~CCN_FORW_REFRESHED;
    return;
  }
  
  if (expires.count() >= std::numeric_limits<int>::max()) {
    this->forw()->expires = std::numeric_limits<int>::max();
  } else {
    this->forw()->expires = static_cast<int>(expires.count());
  }
  this->forw()->flags |= CCN_FORW_REFRESHED;
}

void ForwardingEntry::MakePermanent(void) {
  this->Refresh(std::chrono::seconds::max());
}

PitEntry::PitEntry(interest_entry* ie) : ie_(ie) {
  assert(ie != nullptr);
  assert(ie_ndnfdInterest(ie) != nullptr);
}

Ptr<NamePrefixEntry> PitEntry::npe(void) const {
  return this->New<NamePrefixEntry>(this->name(), this->ie()->ll.npe);
}

bool PitEntry::IsNonceUnique(Ptr<const PitFaceItem> p) {
  return 1 == pfi_unique_nonce(CCNDH, this->ie(), p->p());
}

pit_face_item* PitEntry::SeekPfiInternal(FaceId face, bool create, unsigned flag) {
  if (create) {
    return pfi_seek(CCNDH, this->ie(), static_cast<unsigned>(face), flag);
  }
  
  for (pit_face_item* x = this->ie()->pfl; x != nullptr; x = x->next) {
    if ((x->pfi_flags & flag) != 0 && static_cast<FaceId>(x->faceid) == face) {
      return x;
    }
  }
  return nullptr;
}

void PitEntry::DeletePfiInternal(pit_face_item* p) {
  pfi_destroy(CCNDH, this->ie(), p);
}

std::chrono::microseconds PitEntry::NextEventDelay(bool include_expired) const {
  if (include_expired) {
    int usec = ie_next_usec(CCNDH, this->ie(), nullptr);
    return std::chrono::microseconds(std::max(1, usec));
  }
  
  PitEntry* that = const_cast<PitEntry*>(this);
  ccn_wrappedtime now = CCNDH->wtnow;
  ccn_wrappedtime mn = 600 * WTHZ_value();

  std::for_each(that->beginDownstream(), that->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    if (p->pending()) {
      mn = std::min(mn, p->p()->expiry-now);
    }
  });
  std::for_each(that->beginUpstream(), that->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
    if (!p->IsExpired()) {
      mn = std::min(mn, p->p()->expiry-now);
    }
  });
  
  return std::chrono::microseconds(mn * (1000000 / WTHZ_value()));
}

PitEntry::PitFaceItem::PitFaceItem(Ptr<PitEntry> ie, pit_face_item* p) : ie_(ie), p_(p) {
  assert(ie != nullptr);
  assert(p != nullptr);
}

int PitEntry::PitFaceItem::CompareExpiry(Ptr<const PitFaceItem> a, Ptr<const PitFaceItem> b) {
  return wt_compare(a->p()->expiry, b->p()->expiry);
}

PitUpstreamRecord::PitUpstreamRecord(Ptr<PitEntry> ie, pit_face_item* p) : PitFaceItem(ie, p) {
  assert((p->pfi_flags & CCND_PFI_UPSTREAM) != 0);
}

bool PitUpstreamRecord::IsExpired(void) const {
  return wt_compare(this->p()->expiry, CCNDH->wtnow+1) <= 0;
}

void PitUpstreamRecord::SetExpiry(std::chrono::microseconds t) {
  pfi_set_expiry_from_micros(CCNDH, this->ie()->ie(), this->p(), static_cast<unsigned>(t.count()));
}

PitDownstreamRecord::PitDownstreamRecord(Ptr<PitEntry> ie, pit_face_item* p) : PitFaceItem(ie, p) {
  assert((p->pfi_flags & CCND_PFI_DNSTREAM) != 0);
}

void PitDownstreamRecord::UpdateNonce(Ptr<const InterestMessage> interest) {
  const uint8_t* nonce; size_t nonce_size; uint8_t generated_nonce_buf[TYPICAL_NONCE_SIZE];
  if (interest->parsed()->offset[CCN_PI_B_Nonce] != interest->parsed()->offset[CCN_PI_E_Nonce]) {
    ccn_ref_tagged_BLOB(CCN_DTAG_Nonce, interest->msg(), interest->parsed()->offset[CCN_PI_B_Nonce], interest->parsed()->offset[CCN_PI_E_Nonce], &nonce, &nonce_size);
  } else {
    Ptr<Face> inface = this->global()->facemgr()->GetFace(interest->incoming_face());
    assert(inface != nullptr);
    nonce_size = (CCNDH->noncegen)(CCNDH, inface->ccnd_face(), generated_nonce_buf);
    nonce = generated_nonce_buf;
  }
  
  this->set_p(pfi_set_nonce(CCNDH, this->ie()->ie(), this->p(), nonce, nonce_size));
}

bool PitDownstreamRecord::IsExpired(void) const {
  return wt_compare(this->p()->expiry, CCNDH->wtnow) <= 0;
}

void PitDownstreamRecord::SetExpiryToLifetime(Ptr<const InterestMessage> interest) {
  pfi_set_expiry_from_lifetime(CCNDH, this->ie()->ie(), this->p(), ccn_interest_lifetime(interest->msg(), interest->parsed()));
}

};//namespace ndnfd
