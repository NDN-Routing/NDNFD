#include "nameprefix_table.h"
#include <limits>
extern "C" {
#include <ccn/hashtb.h>
int nameprefix_seek(struct ccnd_handle* h, struct hashtb_enumerator* e, const uint8_t* msg, struct ccn_indexbuf* comps, int ncomps);
struct ccn_forwarding* seek_forwarding(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid);
void update_forward_to(struct ccnd_handle* h, struct nameprefix_entry* npe);
struct ccn_indexbuf* get_outbound_faces(struct ccnd_handle *h, struct face* from, const uint8_t* msg, struct ccn_parsed_interest* pi, struct nameprefix_entry* npe);
void link_interest_entry_to_nameprefix(struct ccnd_handle *h, struct interest_entry *ie, struct nameprefix_entry *npe);
void consume_interest(struct ccnd_handle* h, struct interest_entry* ie);
struct pit_face_item* pfi_seek(struct ccnd_handle* h, struct interest_entry* ie, unsigned faceid, unsigned pfi_flag);
void pfi_destroy(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p);
int ie_next_usec(struct ccnd_handle* h, struct interest_entry* ie, ccn_wrappedtime* expiry);
struct pit_face_item* pfi_set_nonce(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, const uint8_t* nonce, size_t noncesize);
int pfi_nonce_matches(struct pit_face_item* p, const unsigned char* nonce, size_t size);
int pfi_unique_nonce(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p);
void pfi_set_expiry_from_lifetime(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, intmax_t lifetime);
void pfi_set_expiry_from_micros(struct ccnd_handle* h, struct interest_entry* ie, struct pit_face_item* p, unsigned micros);
uint32_t WTHZ_value(void);
int wt_compare(ccn_wrappedtime a, ccn_wrappedtime b);
}
#include "core/scheduler.h"
#include "face/facemgr.h"
#include "message/interest.h"
#include "strategy/layer.h"
extern "C" {

void ndnfd_finalize_interest(struct interest_entry* ie) {
  ndnfd::PitEntry::Finalize(ie);
}

const struct ccn_parsed_interest* ndnfd_ie_pi(const struct interest_entry* ie) {
  ndnfd::InterestMessage* interest = ie_ndnfdInterest(ie);
  return interest->parsed();
}

void ndnfd_attach_npe(struct ccnd_handle* h, struct nameprefix_entry* npe, const uint8_t* name, size_t name_size) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  global->npt()->AttachNpe(npe, name, name_size);
}

void ndnfd_finalize_npe(struct ccnd_handle* h, struct nameprefix_entry* npe) {
  ndnfd::Global* global = ccnd_ndnfdGlobal(h);
  global->npt()->FinalizeNpe(npe);
}

int ndnfd_keep_npe(struct ccnd_handle* h, struct nameprefix_entry* npe) {
  ndnfd::NamePrefixEntry* n = static_cast<ndnfd::NamePrefixEntry*>(npe->ndnfd_npe);
  return n->no_reap() ? 1 : 0;
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
  return static_cast<NamePrefixEntry*>(npe->ndnfd_npe);
}

void NamePrefixTable::ForeachNpe(std::function<ForeachAction(Ptr<NamePrefixEntry>)> f) {
  // ForeachNpe cannot be replaced with STL iterator due to the use of hashtb_enumerator.
  hashtb_enumerator ee; hashtb_enumerator* e = &ee;
  hashtb_start(CCNDH->nameprefix_tab, e);
  for (nameprefix_entry* native = static_cast<nameprefix_entry*>(e->data); native != nullptr; native = static_cast<nameprefix_entry*>(e->data)) {
    Ptr<NamePrefixEntry> npe = static_cast<NamePrefixEntry*>(native->ndnfd_npe);
    ForeachAction act = f(npe);
    if (ForeachAction_break(act)) {
      break;
    }
    hashtb_next(e);
  }
  hashtb_end(e);
}

void NamePrefixTable::AttachNpe(nameprefix_entry* native, const uint8_t* name, size_t name_size) {
  Ptr<Name> n = Name::FromCcnb(name, name_size);
  Ptr<NamePrefixEntry> npe = this->New<NamePrefixEntry>(n, native);
  native->ndnfd_npe = GetPointer(npe);
}

void NamePrefixTable::FinalizeNpe(nameprefix_entry* native) {
  NamePrefixEntry* npe = static_cast<NamePrefixEntry*>(native->ndnfd_npe);
  assert(npe != nullptr);
  npe->Unref();
  native->ndnfd_npe = nullptr;
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
  interest_entry* ie = reinterpret_cast<interest_entry*>(e->data);
  if (res == HT_NEW_ENTRY || ie->ndnfd_consumed == 1) {
    if (res == HT_NEW_ENTRY) ie->serial = ++CCNDH->iserial;
    ie->strategy.birth = ie->strategy.renewed = CCNDH->wtnow;
    ie->strategy.renewals = 0;
    this->Log(kLLDebug, kLCTable, "NamePrefixTable::SeekPit(%s) %s PitEntry(%" PRIu32 ")", npe->name()->ToUri().c_str(), (res == HT_NEW_ENTRY) ? "new" : "reuse", static_cast<uint32_t>(ie->serial));
  }
  if (ie->interest_msg == nullptr) {
    link_interest_entry_to_nameprefix(CCNDH, ie, npe->native());
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

  Ptr<PitEntry> ie1 = this->New<PitEntry>(ie);
  if (res == HT_NEW_ENTRY) ie1->UndoConsumeInternal();
  return ie1;
}

void NamePrefixTable::DeletePit(Ptr<PitEntry> ie) {
  assert(ie != nullptr);
  this->Log(kLLDebug, kLCTable, "NamePrefixTable::DeletePit(%" PRI_PitEntrySerial ")", ie->serial());
  consume_interest(CCNDH, ie->native());
}

NamePrefixEntry::NamePrefixEntry(Ptr<const Name> name, nameprefix_entry* native)
    : name_(name), native_(native), strategy_type_(StrategyType_inherit), strategy_extra_(nullptr), strategy_extra_type_(StrategyType_none) {
  assert(name != nullptr);
  assert(native != nullptr);
}

Ptr<NamePrefixEntry> NamePrefixEntry::Parent(void) const {
  nameprefix_entry* parent = this->native()->parent;
  if (parent == nullptr) return nullptr;
  return static_cast<NamePrefixEntry*>(parent->ndnfd_npe);
}
  
Ptr<NamePrefixEntry> NamePrefixEntry::FibNode(void) const {
  for (Ptr<NamePrefixEntry> n = const_cast<NamePrefixEntry*>(this); n != nullptr; n = n->Parent()) {
    if (n->native()->forwarding != nullptr) return n;
  }
  return nullptr;
}

void NamePrefixEntry::EnsureUpdatedFib(void) const {
  update_forward_to(CCNDH, this->native());
}

std::unordered_set<FaceId> NamePrefixEntry::LookupFib(Ptr<const InterestMessage> interest) const {
  assert(interest != nullptr);
  Ptr<Face> inface = this->global()->facemgr()->GetFace(interest->incoming_face());

  ccn_indexbuf* outbound = get_outbound_faces(CCNDH, inface==nullptr ? nullptr : inface->native(), interest->msg(), const_cast<ccn_parsed_interest*>(interest->parsed()), this->native());

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
    f = seek_forwarding(CCNDH, this->native(), static_cast<unsigned>(faceid));
    ++(CCNDH->forward_to_gen);
  } else {
    for (f = this->native()->forwarding; f != nullptr; f = f->next) {
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
      if (x == this->native()) {
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

Ptr<NamePrefixEntry> NamePrefixEntry::StrategyNode(void) const {
  for (Ptr<NamePrefixEntry> n = const_cast<NamePrefixEntry*>(this); n != nullptr; n = n->Parent()) {
    if (n->strategy_type() != StrategyType_inherit) return n;
  }
  return nullptr;
}

void* NamePrefixEntry::GetStrategyExtraInternal(void) const {
  Ptr<NamePrefixEntry> root = this->StrategyNode();
  assert(root != nullptr);
  if (this->strategy_extra_type() != root->strategy_type()) {
    this->global()->sl()->UpdateNpeExtra(const_cast<NamePrefixEntry*>(this));
  }
  return this->strategy_extra_;
}

ForwardingEntry::ForwardingEntry(Ptr<NamePrefixEntry> npe, ccn_forwarding* native) : npe_(npe), native_(native) {
  assert(npe != nullptr);
  assert(native != nullptr);
}

void ForwardingEntry::Refresh(std::chrono::seconds expires) {
  if (expires < std::chrono::seconds::zero()) {
    this->native()->flags &= ~CCN_FORW_REFRESHED;
    return;
  }
  
  if (expires.count() >= std::numeric_limits<int>::max()) {
    this->native()->expires = std::numeric_limits<int>::max();
  } else {
    this->native()->expires = static_cast<int>(expires.count());
  }
  this->native()->flags |= CCN_FORW_REFRESHED;
}

void ForwardingEntry::MakePermanent(void) {
  this->Refresh(std::chrono::seconds::max());
}

PitEntry::PitEntry(interest_entry* native) : native_(native) {
  assert(native != nullptr);
  assert(ie_ndnfdInterest(native) != nullptr);
}

void PitEntry::Finalize(interest_entry* native) {
  ndnfd::InterestMessage* interest = ie_ndnfdInterest(native);
  interest->Unref();
  
  RttTable* rt = static_cast<RttTable*>(native->ndnfd_rtt_records);
  if (rt != nullptr) {
    delete rt;
  }  
}

Ptr<NamePrefixEntry> PitEntry::npe(void) const {
  return static_cast<NamePrefixEntry*>(this->native()->ll.npe->ndnfd_npe);
}

bool PitEntry::IsNonceUnique(Ptr<const PitFaceItem> p) {
  return 1 == pfi_unique_nonce(CCNDH, this->native(), p->native());
}

Ptr<PitDownstreamRecord> PitEntry::FindPendingDownstream(void) {
  auto it = std::find_if(this->beginDownstream(), this->endDownstream(), [](Ptr<PitDownstreamRecord> p) { return p->pending(); });
  if (it == this->endDownstream()) return nullptr;
  return *it;
}

pit_face_item* PitEntry::SeekPfiInternal(FaceId face, bool create, unsigned flag) {
  if (create) {
    return pfi_seek(CCNDH, this->native(), static_cast<unsigned>(face), flag);
  }
  
  for (pit_face_item* x = this->native()->pfl; x != nullptr; x = x->next) {
    if ((x->pfi_flags & flag) != 0 && static_cast<FaceId>(x->faceid) == face) {
      return x;
    }
  }
  return nullptr;
}

void PitEntry::DeletePfiInternal(pit_face_item* p) {
  pfi_destroy(CCNDH, this->native(), p);
}

std::chrono::microseconds PitEntry::NextEventDelay(bool include_expired) const {
  if (include_expired) {
    int usec = ie_next_usec(CCNDH, this->native(), nullptr);
    return std::chrono::microseconds(std::max(1, usec));
  }
  
  PitEntry* that = const_cast<PitEntry*>(this);
  ccn_wrappedtime now = CCNDH->wtnow;
  ccn_wrappedtime mn = 600 * WTHZ_value();

  std::for_each(that->beginDownstream(), that->endDownstream(), [&] (Ptr<PitDownstreamRecord> p) {
    if (p->pending()) {
      mn = std::min(mn, p->native()->expiry - now);
    }
  });
  std::for_each(that->beginUpstream(), that->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
    if (!p->IsExpired()) {
      mn = std::min(mn, p->native()->expiry - now);
    }
  });
  
  return std::chrono::microseconds(mn * (1000000 / WTHZ_value()));
}

void PitEntry::Consume(void) {
  if (this->consumed()) return;
  this->native()->ndnfd_consumed = 1;
  
  // cancel events
  if (this->native()->ev != nullptr) ccn_schedule_cancel(CCNDH->sched, this->native()->ev);
  if (this->native()->strategy.ev != nullptr) ccn_schedule_cancel(CCNDH->sched, this->native()->strategy.ev);
  this->native()->ev = this->native()->strategy.ev = nullptr;
  
  // schedule deletion on last upstream expiry
  ccn_wrappedtime now = CCNDH->wtnow;
  ccn_wrappedtime last = 0;
  std::for_each(this->beginUpstream(), this->endUpstream(), [&] (Ptr<PitUpstreamRecord> p) {
    if (!p->IsExpired()) {
      last = std::max(last, p->native()->expiry - now);
    }
  });
  Ptr<PitEntry> that = this;
  this->global()->scheduler()->Schedule(std::chrono::microseconds(last * (1000000 / WTHZ_value())), [that]{
    that->global()->npt()->DeletePit(that);
    return Scheduler::kNoMore_NoCleanup;
  }, &this->native()->ev, true);

  // clear pfi
  pit_face_item* next;
  for (pit_face_item* p = this->native()->pfl; p != nullptr; p = next) {
    next = p->next;
    if ((p->pfi_flags & CCND_PFI_PENDING) != 0) {
      Ptr<Face> face = this->global()->facemgr()->GetFace(p->faceid);
      if (face != nullptr) face->native()->pending_interests -= 1;
    }
    free(p);
  }
  this->native()->pfl = nullptr;
}

void PitEntry::UndoConsumeInternal(void) {
  if (!this->consumed()) return;
  this->native()->ndnfd_consumed = 0;
  
  // cancel deletion
  this->global()->scheduler()->Cancel(this->native()->ev);
}

void PitEntry::Renew(void) {
  this->native()->strategy.renewed = CCNDH->wtnow;
  this->native()->strategy.renewals += 1;
}

void PitEntry::RttStart(FaceId upstream) {
  this->RttStartInternal(upstream, std::chrono::microseconds::zero(), nullptr);
}

void PitEntry::RttStartWithExpect(FaceId upstream, std::chrono::microseconds expect, std::function<void()> cb) {
  assert(expect > std::chrono::microseconds::zero());
  assert(!!cb);
  this->RttStartInternal(upstream, expect, cb);
}

void PitEntry::RttStartInternal(FaceId upstream, std::chrono::microseconds expect, std::function<void()> cb) {
  RttTable* rt = static_cast<RttTable*>(this->native()->ndnfd_rtt_records);
  if (rt == nullptr) {
    this->native()->ndnfd_rtt_records = rt = new RttTable();
  }
  RttRecord& rr = (*rt)[upstream];
  rr.start_time_ = CCNDH->wtnow;
  if (expect > std::chrono::microseconds::zero()) {
    SchedulerEvent evt = this->global()->scheduler()->Schedule1(expect, [cb,rt,upstream](SchedulerEvent evt){
      RttRecord& rr = (*rt)[upstream];
      rr.timeout_evts_.erase(evt);
      cb();
      return Scheduler::kNoMore;
    });
    rr.timeout_evts_.insert(evt);
  }
  //this->Log(kLLDebug, kLCTable, "PitEntry(%" PRI_PitEntrySerial ")::RttStart(%" PRI_FaceId ",%" PRIuMAX ")", this->serial(), upstream, static_cast<uintmax_t>(expect.count()));
}

std::chrono::microseconds PitEntry::RttEnd(FaceId upstream) const {
  RttTable* rt = static_cast<RttTable*>(this->native()->ndnfd_rtt_records);
  if (rt == nullptr) return std::chrono::microseconds::min();
  auto rr_it = rt->find(upstream);
  if (rr_it == rt->end()) return std::chrono::microseconds::min();
  RttRecord& rr = rr_it->second;
  std::chrono::microseconds rtt((CCNDH->wtnow - rr.start_time_) * (1000000 / WTHZ_value()));
  //this->Log(kLLDebug, kLCTable, "PitEntry(%" PRI_PitEntrySerial ")::RttEnd(%" PRI_FaceId ") %" PRIuMAX , this->serial(), upstream, static_cast<uintmax_t>(rtt.count()));
  
  for (SchedulerEvent evt : rr.timeout_evts_) {
    this->global()->scheduler()->Cancel(evt);
  }
  rr.timeout_evts_.clear();
  
  return rtt;
}


PitFaceItem::PitFaceItem(Ptr<PitEntry> ie, pit_face_item* native) : ie_(ie), native_(native) {
  assert(ie != nullptr);
  assert(native != nullptr);
}

std::chrono::microseconds PitFaceItem::time_until_expiry(void) const {
  ccn_wrappedtime delta = this->native()->expiry - CCNDH->wtnow;
  return std::chrono::microseconds(delta * 1000000 / WTHZ_value());
}

bool PitFaceItem::IsExpired(void) const {
  return wt_compare(this->native()->expiry, CCNDH->wtnow) <= 0;
}

int PitFaceItem::CompareExpiry(Ptr<const PitFaceItem> a, Ptr<const PitFaceItem> b) {
  return wt_compare(a->native()->expiry, b->native()->expiry);
}

InterestMessage::Nonce PitFaceItem::nonce(void) const {
  InterestMessage::Nonce n;
  n.nonce = this->native()->nonce;
  n.size = this->native()->pfi_flags & CCND_PFI_NONCESZ;
  return n;
}

bool PitFaceItem::NonceEquals(const InterestMessage::Nonce& n) {
  return pfi_nonce_matches(this->native(), n.nonce, n.size) != 0;
}

PitUpstreamRecord::PitUpstreamRecord(Ptr<PitEntry> ie, pit_face_item* native) : PitFaceItem(ie, native) {
  assert(this->GetFlag(CCND_PFI_UPSTREAM));
}

void PitUpstreamRecord::SetExpiry(std::chrono::microseconds t) {
  pfi_set_expiry_from_micros(CCNDH, this->ie()->native(), this->native(), static_cast<unsigned>(t.count()));
}

PitDownstreamRecord::PitDownstreamRecord(Ptr<PitEntry> ie, pit_face_item* native) : PitFaceItem(ie, native) {
  assert(this->GetFlag(CCND_PFI_DNSTREAM));
}

void PitDownstreamRecord::UpdateNonce(Ptr<const InterestMessage> interest) {
  InterestMessage::Nonce n = interest->nonce();
  uint8_t generated_nonce_buf[TYPICAL_NONCE_SIZE];
  if (n.size == 0) {
    Ptr<Face> in_face = this->global()->facemgr()->GetFace(interest->incoming_face());
    assert(in_face != nullptr);
    n.size = (CCNDH->noncegen)(CCNDH, in_face->native(), generated_nonce_buf);
    n.nonce = generated_nonce_buf;
  }
  this->set_native(pfi_set_nonce(CCNDH, this->ie()->native(), this->native(), n.nonce, n.size));
}

void PitDownstreamRecord::SetExpiryToLifetime(Ptr<const InterestMessage> interest) {
  pfi_set_expiry_from_lifetime(CCNDH, this->ie()->native(), this->native(), ccn_interest_lifetime(interest->msg(), interest->parsed()));
}

};//namespace ndnfd
