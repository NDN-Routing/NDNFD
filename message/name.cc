#include "name.h"
extern "C" {
#include <ccn/ccn.h>
#include <ccn/uri.h>
}
namespace ndnfd {

Ptr<Name> Name::FromCcnb(uint8_t* buf, size_t length) {
  ccn_buf_decoder decoder;
  ccn_buf_decoder* d = ccn_buf_decoder_start(&decoder, buf, length);

  if (!ccn_buf_match_dtag(d, CCN_DTAG_Name)) return nullptr;
  ccn_buf_advance(d);
  Ptr<Name> n = new Name();
  while (ccn_buf_match_dtag(d, CCN_DTAG_Component)) {
    ccn_buf_advance(d);
    const uint8_t* comp = nullptr;
    size_t compsize = 0;
    if (ccn_buf_match_blob(d, &comp, &compsize)) ccn_buf_advance(d);
    ccn_buf_check_close(d);
    if (d->decoder.state < 0) return nullptr;
    n->comps_.push_back(Component(comp, compsize));
  }
  return n;
}

Ptr<Name> Name::FromUri(std::string uri) {
  ccn_charbuf* c = ccn_charbuf_create();
  int res = ccn_name_from_uri(c, uri.c_str());
  Ptr<Name> n = nullptr;
  if (res >= 0) {
    n = Name::FromCcnb(c->buf, c->length);
  }
  ccn_charbuf_destroy(&c);
  return n;
}

Ptr<Name> Name::Append(const Component& component) const {
  Ptr<Name> n = new Name(this->comps());
  n->comps_.push_back(component);
  return n;
}

Ptr<Name> Name::GetPrefix(uint16_t first_n) const {
  if (first_n > this->n_comps()) {
    return nullptr;
  }
  Ptr<Name> n = new Name(this->comps());
  n->comps_.resize(first_n);
  return n;
}

Ptr<Name> Name::StripSuffix(uint16_t remove_n) const {
  if (remove_n > this->n_comps()) {
    return nullptr;
  }
  return this->GetPrefix(this->n_comps() - remove_n);
}

bool Name::ToCcnb(ccn_charbuf* c) const {
  int res = ccn_name_init(c);
  if (res != 0) return false;
  for (const Component& comp : this->comps()) {
    res = ccn_name_append(c, comp.data(), comp.size());
    if (res != 0) return false;
  }
  return res;
}

std::string Name::ToUri(void) const {
  ccn_charbuf* ccnb = ccn_charbuf_create();
  ccn_charbuf* uri = ccn_charbuf_create();
  if (this->ToCcnb(ccnb)) {
    ccn_uri_append(uri, ccnb->buf, ccnb->length, 0);
  }
  std::string u(reinterpret_cast<const char*>(uri->buf), uri->length);
  ccn_charbuf_destroy(&ccnb);
  ccn_charbuf_destroy(&uri);
  return u;
}

bool Name::Equals(const Ptr<Name> other) const {
  if (this->n_comps() != other->n_comps()) return false;
  for (uint16_t i = 0; i < this->n_comps(); ++i) {
    if (this->comps_[i] != other->comps_[i]) return false;
  }
  return true;
}

bool Name::IsPrefixOf(const Ptr<Name> other) const {
  if (this->n_comps() > other->n_comps()) return false;
  for (uint16_t i = 0; i < this->n_comps(); ++i) {
    if (this->comps_[i] != other->comps_[i]) return false;
  }
  return true;
}

};//namespace ndnfd
