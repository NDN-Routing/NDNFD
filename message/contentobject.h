#ifndef NDNFD_MESSAGE_CONTENTOBJECT_H_
#define NDNFD_MESSAGE_CONTENTOBJECT_H_
#include "util/defs.h"
#include "message/ccnbmessage.h"
#include "message/name.h"
namespace ndnfd {

//represents a ContentObject
//not currently used
class ContentObject : public CcnbMessage {
  public:
    static const MessageType kType = 1002;
    virtual MessageType type(void) const { return ContentObject::kType; }
    
    const Name& name(void) const;
    ccn_parsed_ContentObject parsed;

  private:
    DISALLOW_COPY_AND_ASSIGN(ContentObject);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CONTENTOBJECT_H_
