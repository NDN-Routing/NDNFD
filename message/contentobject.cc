#include "contentobject.h"
namespace ndnfd {

MessageType_def(ContentObjectMessage);

Ptr<ContentObjectMessage> ContentObjectMessage::Parse(const uint8_t* msg, size_t length) {
  Ptr<ContentObjectMessage> m = new ContentObjectMessage(msg, length);
  m->comps_ = ccn_indexbuf_create();
  int res = ccn_parse_ContentObject(msg, length, &m->parsed_, m->comps_);
  if (res < 0) return nullptr;
  return m;
}

ContentObjectMessage::ContentObjectMessage(const uint8_t* msg, size_t length)
    : CcnbMessage(msg, length), has_explicit_digest_(false) {
}

ContentObjectMessage::~ContentObjectMessage(void) {
  if (this->comps_ != nullptr) ccn_indexbuf_destroy(&this->comps_);
}

Ptr<Name> ContentObjectMessage::name(void) const {
  if (this->name_ == nullptr) {
    ContentObjectMessage* that = const_cast<ContentObjectMessage*>(this);
    Ptr<Name> n = Name::FromCcnb(that->msg() + that->parsed()->offset[CCN_PCO_B_Name], that->parsed()->offset[CCN_PCO_E_Name] - that->parsed()->offset[CCN_PCO_B_Name]);
    that->name_ = n;
  }
  return this->name_;
}

std::tuple<const uint8_t*,size_t> ContentObjectMessage::payload(void) const {
  const uint8_t* value; size_t value_size;
  int res = ccn_content_get_value(this->msg(), this->length(), this->parsed(), &value, &value_size);
  if (res != 0) return std::forward_as_tuple(nullptr, 0);
  return std::forward_as_tuple(value, value_size);
}

Ptr<ContentObjectMessage> ContentObjectMessage::AddExplicitDigest(void) const {
  if (this->has_explicit_digest()) return const_cast<ContentObjectMessage*>(this);

  ccn_digest_ContentObject(this->msg(), const_cast<ccn_parsed_ContentObject*>(this->parsed()));
  if (this->parsed()->digest_bytes != 32) return nullptr;

  auto i = this->parsed()->offset[CCN_PCO_E_ComponentLast];
  ccn_charbuf* cb = ccn_charbuf_create();
  ccn_charbuf_append(cb, this->msg(), i);
  ccn_charbuf_append_tt(cb, CCN_DTAG_Component, CCN_DTAG);
  ccn_charbuf_append_tt(cb, this->parsed()->digest_bytes, CCN_BLOB);
  ccn_charbuf_append(cb, this->parsed()->digest, this->parsed()->digest_bytes);
  ccn_charbuf_append_closer(cb);
  ccn_charbuf_append(cb, this->msg() + i, this->length() - i);

  Ptr<Buffer> b = Buffer::Adopt(&cb);
  Ptr<ContentObjectMessage> m = ContentObjectMessage::Parse(b->data(), b->length());
  assert(m != nullptr);
  m->set_source_buffer(b);
  m->has_explicit_digest_ = true;
  return m;
}

};//namespace ndnfd
