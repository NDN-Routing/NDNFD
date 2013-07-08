#ifndef NDNFD_MESSAGE_INTEREST_H_
#define NDNFD_MESSAGE_INTEREST_H_
#include "message/ccnb.h"
#include "message/name.h"
namespace ndnfd {

// An InterestMessage represents a decoded Interest.
class InterestMessage : public CcnbMessage {
 public:
  struct Nonce {
    const uint8_t* nonce;
    size_t size;
  };
  
  InterestMessage(const uint8_t* msg, size_t length, const ccn_parsed_interest* parsed);
  virtual ~InterestMessage(void);
  MessageType_decl;
  
  // Parse parses a buffer and returns parsed InterestMessage,
  // or null if buffer is not a CCNB Interest.
  static Ptr<InterestMessage> Parse(const uint8_t* msg, size_t length);
  
  // Interest Name
  Ptr<Name> name(void) const;
  
  // ccn_parsed_interest structure
  const ccn_parsed_interest* parsed(void) const { return &this->parsed_; }
  
  // name components indexbuf structure
  // This is available only from InterestMessage::Parse.
  const ccn_indexbuf* comps(void) const { return this->comps_; }
  
  // the nonce, or (nullptr,0) if no nonce exists
  Nonce nonce(void) const;

 private:
  ccn_parsed_interest parsed_;
  Ptr<Name> name_;
  ccn_indexbuf* comps_;

  InterestMessage(const uint8_t* msg, size_t length);
  DISALLOW_COPY_AND_ASSIGN(InterestMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_INTEREST_H_
