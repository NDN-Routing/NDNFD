#ifndef NDNFD_MESSAGE_INTEREST_H_
#define NDNFD_MESSAGE_INTEREST_H_
#include "message/ccnb.h"
#include "message/name.h"
namespace ndnfd {

// An InterestMessage represents a decoded Interest.
class InterestMessage : public CcnbMessage {
 public:
  static const MessageType kType = 1001;
  virtual MessageType type(void) const { return InterestMessage::kType; }
  
  // Parse parses a buffer and returns parsed InterestMessage,
  // or null if buffer is not a CCNB Interest.
  static Ptr<InterestMessage> Parse(uint8_t* msg, size_t length);
  
  // Interest Name
  Ptr<Name> name(void) const;
  
  // ccn_parsed_interest structure
  const ccn_parsed_interest* parsed(void) const { return &this->parsed_; }

 private:
  ccn_parsed_interest parsed_;
  Ptr<Name> name_;

  InterestMessage(uint8_t* msg, size_t length);
  DISALLOW_COPY_AND_ASSIGN(InterestMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_INTEREST_H_
