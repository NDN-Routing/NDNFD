#include "interest.h"
namespace ndnfd {

MessageType_def(InterestMessage);

InterestMessage::InterestMessage(const uint8_t* msg, size_t length, const ccn_parsed_interest* parsed)
    : CcnbMessage(msg, length) {
  assert(parsed != nullptr);
  memcpy(&this->parsed_, parsed, sizeof(this->parsed_));
  this->comps_ = nullptr;
}

InterestMessage::InterestMessage(const uint8_t* msg, size_t length) : CcnbMessage(msg, length) {}

InterestMessage::~InterestMessage(void) {
  if (this->comps_ != nullptr) ccn_indexbuf_destroy(&this->comps_);
}

Ptr<InterestMessage> InterestMessage::Parse(const uint8_t* msg, size_t length) {
  Ptr<InterestMessage> m = new InterestMessage(msg, length);
  m->comps_ = ccn_indexbuf_create();
  int res = ccn_parse_interest(msg, length, &m->parsed_, m->comps_);
  if (res < 0) return nullptr;
  return m;
}

Ptr<Name> InterestMessage::name(void) const {
  if (this->name_ == nullptr) {
    InterestMessage* that = const_cast<InterestMessage*>(this);
    Ptr<Name> n = Name::FromCcnb(that->msg() + that->parsed()->offset[CCN_PI_B_Name], that->parsed()->offset[CCN_PI_E_Name] - that->parsed()->offset[CCN_PI_B_Name]);
    that->name_ = n;
  }
  return this->name_;
}

InterestMessage::Nonce InterestMessage::nonce(void) const {
  Nonce n;
  if (this->parsed()->offset[CCN_PI_B_Nonce] != this->parsed()->offset[CCN_PI_E_Nonce]) {
    ccn_ref_tagged_BLOB(CCN_DTAG_Nonce, this->msg(), this->parsed()->offset[CCN_PI_B_Nonce], this->parsed()->offset[CCN_PI_E_Nonce], &n.nonce, &n.size);
  } else {
    n.nonce = nullptr;
    n.size = 0;
  }
  return n;
}

};//namespace ndnfd
