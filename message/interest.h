#ifndef NDNFD_MESSAGE_INTEREST_H_
#define NDNFD_MESSAGE_INTEREST_H_
#include "util/defs.h"
#include "message/ccnbmessage.h"
#include "message/name.h"
namespace ndnfd {

//represents an Interest
//not currently used
class Interest : public CcnbMessage {
  public:
    static const MessageType kType = 1001;
    virtual MessageType type(void) const { return Interest::kType; }
    
    const Name& name(void) const;
    ccn_parsed_interest parsed;

  private:
    DISALLOW_COPY_AND_ASSIGN(Interest);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_INTEREST_H_
