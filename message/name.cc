#include "name.h"
extern "C" {
#include <ccn/ccn.h>
#include <ccn/uri.h>
}
namespace ndnfd {

Ptr<Name> Name::FromCcnb(const uint8_t* buf, size_t length) {
  ccn_buf_decoder decoder;
  ccn_buf_decoder* d = ccn_buf_decoder_start(&decoder, buf, length);

  Ptr<Name> n = new Name();
  if (ccn_buf_match_dtag(d, CCN_DTAG_Name)) ccn_buf_advance(d);
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

Ptr<Name> Name::FromUri(const std::string& uri) {
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

std::basic_string<uint8_t> Name::ToCcnb(bool tag_Name, ccn_indexbuf* comps) const {
  ccn_charbuf* c = ccn_charbuf_create();
  ccn_name_init(c);
  size_t start = tag_Name ? 0 : c->length - 1;
  if (comps != nullptr) comps->n = 0;
  for (const Component& comp : this->comps()) {
    if (comps != nullptr) ccn_indexbuf_append_element(comps, c->length - 1 - start);
    ccn_name_append(c, comp.data(), comp.size());
  }
  if (comps != nullptr) ccn_indexbuf_append_element(comps, c->length - 1 - start);
  size_t end = tag_Name ? c->length : c->length - 1;
  std::basic_string<uint8_t> s(c->buf + start, end - start);
  ccn_charbuf_destroy(&c);
  return s;
}

std::string Name::ToUri(void) const {
  std::basic_string<uint8_t> ccnb = this->ToCcnb();
  ccn_charbuf* uri = ccn_charbuf_create();
  ccn_uri_append(uri, ccnb.data(), ccnb.size(), 0);
  std::string u(reinterpret_cast<const char*>(uri->buf), uri->length);
  ccn_charbuf_destroy(&uri);
  return u;
}

bool Name::Equals(Ptr<const Name> other) const {
  if (this->n_comps() != other->n_comps()) return false;
  for (uint16_t i = 0; i < this->n_comps(); ++i) {
    if (this->comps_[i] != other->comps_[i]) return false;
  }
  return true;
}

bool Name::IsPrefixOf(Ptr<const Name> other) const {
  if (this->n_comps() > other->n_comps()) return false;
  for (uint16_t i = 0; i < this->n_comps(); ++i) {
    if (this->comps_[i] != other->comps_[i]) return false;
  }
  return true;
}

};//namespace ndnfd
