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
  virtual MessageType type(void) const { return ContentObject::kType; }
  
  const Name& name(void) const;
  ::ccn_parsed_ContentObject parsed;

 private:
  DISALLOW_COPY_AND_ASSIGN(ContentObjectMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CONTENTOBJECT_H_
