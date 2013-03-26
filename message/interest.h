#ifndef NDNFD_MESSAGE_INTEREST_H_
#define NDNFD_MESSAGE_INTEREST_H_
#include "message/ccnb.h"
#include "message/name.h"
namespace ndnfd {

// An InterestMessage represents a decoded Interest.
// This is not currently used.
class InterestMessage : public CcnbMessage {
 public:
  static const MessageType kType = 1001;
  virtual MessageType type(void) const { return InterestMessage::kType; }
  static Ptr<InterestMessage> Parse(uint8_t* msg, size_t length);
  
  Ptr<Name> name(void) const;
  const ccn_parsed_interest* parsed() const { return &this->parsed_; }

 private:
  ccn_parsed_interest parsed_;
  Ptr<Name> name_;

  InterestMessage(uint8_t* msg, size_t length);
  DISALLOW_COPY_AND_ASSIGN(InterestMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_INTEREST_H_
