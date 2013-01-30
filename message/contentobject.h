#ifndef CCND2_MESSAGE_CONTENTOBJECT_H_
#define CCND2_MESSAGE_CONTENTOBJECT_H_
#include "util/defs.h"
#include "message/ccnbmessage.h"
#include "message/name.h"
namespace ccnd2 {

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

};//namespace ccnd2
#endif//CCND2_MESSAGE_CONTENTOBJECT_H_
