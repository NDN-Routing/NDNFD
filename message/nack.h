#ifndef NDNFD_MESSAGE_NACK_H_
#define NDNFD_MESSAGE_NACK_H_
#include "message/interest.h"
namespace ndnfd {

enum class NackCode {
  kDuplicate  = 161,
  kCongestion = 162,
  kNoData     = 163,
};

bool NackCode_valid(NackCode code);
const char* NackCode_string(NackCode code);

// An NackMessage represents a decoded Nack.
class NackMessage : public CcnbMessage {
 public:
  //NackMessage(const uint8_t* msg, size_t length, const ccn_parsed_interest* parsed);
  virtual ~NackMessage(void) {}
  MessageType_decl;
  
  // Parse parses a buffer and returns parsed NackMessage,
  // or null if buffer is not a CCNB Nack.
  static Ptr<NackMessage> Parse(const uint8_t* msg, size_t length);
  
  // Create creates a NackMessage from code and InterestMessage.
  static Ptr<Buffer> Create(NackCode code, Ptr<const InterestMessage> interest, const InterestMessage::Nonce& nonce);
  
  NackCode code(void) const { return this->code_; }
  
  // the Interest
  Ptr<InterestMessage> interest(void) const { return this->interest_; }

 private:
  NackCode code_;
  Ptr<InterestMessage> interest_;

  NackMessage(const uint8_t* msg, size_t length);
  DISALLOW_COPY_AND_ASSIGN(NackMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_NACK_H_
