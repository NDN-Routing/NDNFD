#include "interest.h"
namespace ndnfd {

const MessageType InterestMessage::kType;

InterestMessage::InterestMessage(const uint8_t* msg, size_t length, const ccn_parsed_interest* parsed)
    : CcnbMessage(msg, length) {
  assert(parsed != nullptr);
  memcpy(&this->parsed_, parsed, sizeof(this->parsed_));
}

Ptr<InterestMessage> InterestMessage::Parse(const uint8_t* msg, size_t length) {
  Ptr<InterestMessage> m = new InterestMessage(msg, length);
  int res = ccn_parse_interest(msg, length, &m->parsed_, nullptr);
  if (res < 0) return nullptr;
  return m;
}

InterestMessage::InterestMessage(const uint8_t* msg, size_t length) : CcnbMessage(msg, length) {}

Ptr<Name> InterestMessage::name(void) const {
  if (this->name_ == nullptr) {
    InterestMessage* that = const_cast<InterestMessage*>(this);
    Ptr<Name> n = Name::FromCcnb(that->msg() + that->parsed()->offset[CCN_PI_B_Name], that->parsed()->offset[CCN_PI_E_Name] - that->parsed()->offset[CCN_PI_B_Name]);
    that->name_ = n;
  }
  return this->name_;
}

};//namespace ndnfd
