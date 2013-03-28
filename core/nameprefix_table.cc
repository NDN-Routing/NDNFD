#include "nameprefix_table.h"
extern "C" {
#include <ccn/hashtb.h>
int nameprefix_seek(struct ccnd_handle* h, struct hashtb_enumerator* e, const uint8_t* msg, struct ccn_indexbuf* comps, int ncomps);
struct ccn_forwarding* seek_forwarding(struct ccnd_handle* h, struct nameprefix_entry* npe, unsigned faceid);
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

NamePrefixEntry::NamePrefixEntry(Ptr<const Name> name, nameprefix_entry* npe) : name_(name), npe_(npe) {
  assert(name != nullptr);
  assert(npe != nullptr);
}

Ptr<ForwardingEntry> NamePrefixEntry::SeekForwardingInternal(FaceId faceid, bool create) {
  ccn_forwarding* f;
  
  if (create) {
    f = seek_forwarding(this->global()->ccndh(), this->npe(), static_cast<unsigned>(faceid));
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

};//namespace ndnfd
