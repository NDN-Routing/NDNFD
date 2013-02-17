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
  
  const Name& name(void) const;
  ccn_parsed_interest parsed;

 private:
  DISALLOW_COPY_AND_ASSIGN(InterestMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_INTEREST_H_
