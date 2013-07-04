#ifndef NDNFD_MESSAGE_CONTENTOBJECT_H_
#define NDNFD_MESSAGE_CONTENTOBJECT_H_
#include "message/ccnb.h"
#include "message/name.h"
namespace ndnfd {

// A ContentObjectMessage represents a decoded ContentObject.
// This is not currently used.
class ContentObjectMessage : public CcnbMessage {
 public:
  MessageType_decl;
  virtual ~ContentObjectMessage(void);
  
  // Parse parses a buffer and returns parsed ContentObjectMessage,
  // or null if buffer is not a CCNB ContentObject.
  static Ptr<ContentObjectMessage> Parse(const uint8_t* msg, size_t length);
  
  // ContentObject Name
  Ptr<Name> name(void) const;
  
  // ccn_parsed_ContentObject structure
  const ccn_parsed_ContentObject* parsed(void) const { return &this->parsed_; }
  
  // Content blob
  std::tuple<const uint8_t*,size_t> payload(void) const;

  // name components indexbuf structure
  const ccn_indexbuf* comps(void) const { return this->comps_; }

 private:
  ccn_parsed_ContentObject parsed_;
  Ptr<Name> name_;
  ccn_indexbuf* comps_;
  
  ContentObjectMessage(const uint8_t* msg, size_t length);
  DISALLOW_COPY_AND_ASSIGN(ContentObjectMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CONTENTOBJECT_H_
