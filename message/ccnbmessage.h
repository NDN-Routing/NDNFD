#ifndef NDNFD_MESSAGE_CCNBMESSAGE_H_
#define NDNFD_MESSAGE_CCNBMESSAGE_H_
#include "util/defs.h"
extern "C" {
#include <ccn/ccn.h>
}
#include "message/message.h"
namespace ndnfd {

//unparsed ccnb message
//can pass to process_input_message()
class CcnbMessage : public Message {
  public:
    static const MessageType kType = 1099;
    virtual MessageType type(void) const { return CcnbMessage::kType; }
    
    //CcnbMessage does not own this buffer
    uint8_t* msg;
    size_t size;

  private:
    DISALLOW_COPY_AND_ASSIGN(CcnbMessage);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_CCNBMESSAGE_H_
