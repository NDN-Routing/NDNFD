#ifndef NDNFD_MESSAGE_CONTENTOBJECT_H_
#define NDNFD_MESSAGE_CONTENTOBJECT_H_
#include "message/ccnb.h"
#include "message/name.h"
namespace ndnfd {

// A ContentObjectMessage represents a decoded ContentObject.
// This is not currently used.
class ContentObjectMessage : public CcnbMessage {
 public:
  static const MessageType kType = 1002;
  virtual MessageType type(void) const { return ContentObjectMessage::kType; }
  static Ptr<ContentObjectMessage> Parse(uint8_t* msg, size_t length);
  
  Ptr<Name> name(void) const;
  std::tuple<const uint8_t*,size_t> payload() const;
  const ccn_parsed_ContentObject* parsed() const { return &this->parsed_; }

 private:
  ccn_parsed_ContentObject parsed_;
  Ptr<Name> name_;
  
  ContentObjectMessage(uint8_t* msg, size_t length);
  DISALLOW_COPY_AND_ASSIGN(ContentObjectMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CONTENTOBJECT_H_
