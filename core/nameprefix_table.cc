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
int wt_compare(ccn_wrappedtime a, ccn_wrappedtime b);
}
#include "face/facemgr.h"
namespace ndnfd {

Ptr<NamePrefixEntry> NamePrefixTable::SeekInternal(Ptr<const Name> name, bool create) {
  ccn_indexbuf* comps = ccn_indexbuf_create();
  std::basic_string<uint8_t> name_ccnb = name->ToCcnb(false, comps);

  nameprefix_entry* npe;
  if (create) {
    hashtb_enumerator ee; hashtb_enumerator* e = &ee;
    hashtb_start(this->global()->ccndh()->nameprefix_tab, e);
    nameprefix_seek(this->global()->ccndh(), e, name_ccnb.data(), comps, name->n_comps());
    npe = reinterpret_cast<nameprefix_entry*>(e->data);
    hashtb_end(e);
  } else {
    npe = reinterpret_cast<nameprefix_entry*>(hashtb_lookup(this->global()->ccndh()->nameprefix_tab, name_ccnb.data(), name_ccnb.size()));
  }

  ccn_indexbuf_destroy(&comps);
  if (npe == nullptr) return nullptr;
  return this->New<NamePrefixEntry>(name, npe);
}

Ptr<PitEntry> NamePrefixTable::GetPit(Ptr<const InterestMessage> interest) {
  interest_entry* ie = reinterpret_cast<interest_entry*>(hashtb_lookup(this->global()->ccndh()->interest_tab, interest->msg(), interest->parsed()->offset[CCN_PI_B_InterestLifetime]));
  if (ie == nullptr) return nullptr;
  return this->New<PitEntry>(interest->name(), ie);
}

Ptr<PitEntry> NamePrefixTable::SeekPit(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  assert(interest != nullptr);
  assert(npe != nullptr);
  assert(interest->name()->Equals(npe->name()));
  ccnd_handle* h = this->global()->ccndh();
  hashtb_enumerator ee; hashtb_enumerator* e = &ee;
  hashtb_start(h->interest_tab, e);

  int res = hashtb_seek(e, interest->msg(), interest->parsed()->offset[CCN_PI_B_InterestLifetime], 1);
  interest_entry* ie; ie = reinterpret_cast<interest_entry*>(e->data);
  if (res == HT_NEW_ENTRY) {
    ie->serial = ++h->iserial;
    ie->strategy.birth = ie->strategy.renewed = h->wtnow;
    ie->strategy.renewals = 0;
    this->Log(kLLDebug, kLCStrategy, "NamePrefixTable::SeekPit(%s) new PitEntry(%" PRIu32 ")", npe->name()->ToUri().c_str(), static_cast<uint32_t>(ie->serial));
  }
  if (ie->interest_msg == nullptr) {
    link_interest_entry_to_nameprefix(h, ie, npe->npe());
    ie->interest_msg = reinterpret_cast<const uint8_t*>(e->key);
    ie->size = interest->parsed()->offset[CCN_PI_B_InterestLifetime] + 1;
    const_cast<uint8_t*>(ie->interest_msg) [ie->size-1] = '\0';//set last byte to </Interest>
  }

  hashtb_end(e);
  if (ie == nullptr) return nullptr;
  return this->New<PitEntry>(interest->name(), ie);
}

void NamePrefixTable::DeletePit(Ptr<PitEntry> ie) {
  assert(ie != nullptr);
  consume_interest(this->global()->ccndh(), ie->ie());
}

NamePrefixEntry::NamePrefixEntry(Ptr<const Name> name, nameprefix_entry* npe) : name_(name), npe_(npe) {
  assert(name != nullptr);
  assert(npe != nullptr);
}

Ptr<NamePrefixEntry> NamePrefixEntry::Parent(void) const {
  nameprefix_entry* parent = this->npe()->parent;
  if (parent == nullptr) return nullptr;
  return new NamePrefixEntry(this->name()->StripSuffix(1), parent);
}
  
Ptr<NamePrefixEntry> NamePrefixEntry::FibNode(void) const {
  for (Ptr<NamePrefixEntry> n = const_cast<NamePrefixEntry*>(this); n != nullptr; n = n->Parent()) {
    if (n->npe()->forwarding != nullptr) return n;
  }
  return nullptr;
}

void NamePrefixEntry::EnsureUpdatedFib(void) const {
  update_forward_to(this->global()->ccndh(), this->npe());
}

std::unordered_set<FaceId> NamePrefixEntry::LookupFib(Ptr<const InterestMessage> interest) const {
  assert(interest != nullptr);
  Ptr<Face> inface = this->global()->facemgr()->GetFace(interest->incoming_face());
  assert(inface != nullptr);

  ccn_indexbuf* outbound = get_outbound_faces(this->global()->ccndh(), inface->ccnd_face(), interest->msg(), const_cast<ccn_parsed_interest*>(interest->parsed()), this->npe());

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
    f = seek_forwarding(this->global()->ccndh(), this->npe(), static_cast<unsigned>(faceid));
    ++(this->global()->ccndh()->forward_to_gen);
  } else {
    for (f = this->npe()->forwarding; f != nullptr; f = f->next) {
      if (static_cast<FaceId>(f->faceid) == faceid) break;
    }
  }
  
  if (f == nullptr) return nullptr;
  return this->New<ForwardingEntry>(this, f);
}

void NamePrefixEntry::ForeachPit(std::function<void(Ptr<PitEntry>)> f) {
  hashtb_enumerator ee; hashtb_enumerator* e = &ee;
  hashtb_start(this->global()->ccndh()->interest_tab, e);
  for (interest_entry* ie = static_cast<interest_entry*>(e->data); ie != nullptr; ie = static_cast<interest_entry*>(e->data)) {
    for (nameprefix_entry* x = ie->ll.npe; x != nullptr; x = x->parent) {
      if (x == this->npe()) {
        Ptr<PitEntry> ie1 = this->New<PitEntry>(this->name(), ie);
        f(ie1);
        break;
      }
    }
    hashtb_next(e);
  }
  hashtb_end(e);
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

PitEntry::PitEntry(Ptr<const Name> name, interest_entry* ie) : name_(name), ie_(ie) {
  assert(name != nullptr);
  assert(ie != nullptr);
}

Ptr<NamePrefixEntry> PitEntry::npe(void) const {
  return this->New<NamePrefixEntry>(this->name(), this->ie()->ll.npe);
}

Ptr<const InterestMessage> PitEntry::interest(void) const {
  Ptr<InterestMessage> interest = InterestMessage::Parse(this->ie()->interest_msg, this->ie()->size);
  assert(interest != nullptr);
  return interest;
}

pit_face_item* PitEntry::SeekPfiInternal(FaceId face, bool create, unsigned flag) {
  if (create) {
    return pfi_seek(this->global()->ccndh(), this->ie(), static_cast<unsigned>(face), flag);
  }
  
  for (pit_face_item* x = this->ie()->pfl; x != nullptr; x = x->next) {
    if ((x->pfi_flags & flag) != 0 && static_cast<FaceId>(x->faceid) == face) {
      return x;
    }
  }
  return nullptr;
}

void PitEntry::ForeachInternal(std::function<ForeachAction(pit_face_item*)> f, unsigned flag) {
  pit_face_item* next = nullptr;
  for (pit_face_item* x = this->ie()->pfl; x != nullptr; x = next) {
    next = x->next;
    if ((x->pfi_flags & flag) != 0) {
      ForeachAction act = f(x);
      if ((static_cast<int>(act) & static_cast<int>(ForeachAction::kDelete)) != 0) {
        pfi_destroy(this->global()->ccndh(), this->ie(), x);
      }
      if ((static_cast<int>(act) & static_cast<int>(ForeachAction::kBreak)) != 0) {
        return;
      }
    }
  }
}

std::chrono::microseconds PitEntry::NextEventDelay(bool include_expired) const {
  if (include_expired) {
    int usec = ie_next_usec(this->global()->ccndh(), this->ie(), nullptr);
    return std::chrono::microseconds(usec);
  }

  ccn_wrappedtime now = this->global()->ccndh()->wtnow;
  ccn_wrappedtime mn = 600 * WTHZ_value();

  const_cast<PitEntry*>(this)->ForeachDownstream([&] (pit_face_item* p) ->ForeachAction {
    if ((p->pfi_flags & CCND_PFI_PENDING) != 0) {
      mn = std::min(mn, p->expiry-now);
    }
    return ForeachAction::kNone;
  });
  const_cast<PitEntry*>(this)->ForeachUpstream([&] (pit_face_item* p) ->ForeachAction {
    if (wt_compare(now+1, p->expiry) < 0) {
      mn = std::min(mn, p->expiry-now);
    }
    return ForeachAction::kNone;
  });
  
  return std::chrono::microseconds(mn * (1000000 / WTHZ_value()));
}

};//namespace ndnfd
