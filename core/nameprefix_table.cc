#include "nameprefix_table.h"
#include <limits>
extern "C" {
#include <ccn/hashtb.h>
int nameprefix_seek(struct ccnd_handle* h, struct hashtb_enumerator* e, const uint8_t* msg, struct ccn_indexbuf* comps, int ncomps);
struct ccn_forwarding* seek_forwarding(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid);
void link_interest_entry_to_nameprefix(struct ccnd_handle *h, struct interest_entry *ie, struct nameprefix_entry *npe);
}
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
  return this->New<PitEntry>(ie);
}

Ptr<PitEntry> NamePrefixTable::SeekPit(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe) {
  assert(interest != nullptr);
  assert(npe != nullptr);
  ccnd_handle* h = this->global()->ccndh();
  hashtb_enumerator ee; hashtb_enumerator* e = &ee;
  hashtb_start(h->interest_tab, e);

  int res = hashtb_seek(e, interest->msg(), interest->parsed()->offset[CCN_PI_B_InterestLifetime], 1);
  interest_entry* ie; ie = reinterpret_cast<interest_entry*>(e->data);
  if (res == HT_NEW_ENTRY) {
    ie->serial = ++h->iserial;
    ie->strategy.birth = ie->strategy.renewed = h->wtnow;
    ie->strategy.renewals = 0;
  }
  if (ie->interest_msg == nullptr) {
    link_interest_entry_to_nameprefix(h, ie, npe->npe());
    ie->interest_msg = reinterpret_cast<const uint8_t*>(e->key);
    ie->size = interest->parsed()->offset[CCN_PI_B_InterestLifetime] + 1;
    const_cast<uint8_t*>(ie->interest_msg) [ie->size-1] = '\0';//set last byte to </Interest>
  }

  hashtb_end(e);
  if (ie == nullptr) return nullptr;
  return this->New<PitEntry>(ie);
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
}

};//namespace ndnfd
