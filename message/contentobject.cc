#include "contentobject.h"
namespace ndnfd {

Ptr<ContentObjectMessage> ContentObjectMessage::Parse(uint8_t* msg, size_t length) {
  Ptr<ContentObjectMessage> m = new ContentObjectMessage(msg, length);
  int res = ccn_parse_ContentObject(msg, length, &m->parsed_, nullptr);
  if (res < 0) return nullptr;
  return m;
}

ContentObjectMessage::ContentObjectMessage(uint8_t* msg, size_t length) : CcnbMessage(msg, length) {}

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

};//namespace ndnfd
