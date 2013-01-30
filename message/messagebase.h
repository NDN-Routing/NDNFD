#ifndef NDNFD_MESSAGE_MESSAGEBASE_H_
#define NDNFD_MESSAGE_MESSAGEBASE_H_
#include "util/defs.h"
namespace ndnfd {

//represents the type of a MessageBase subclass
//each subclass of MessageBase should have a unique MessageType
typedef uint16_t MessageType;

//base for most interactions between elements
class MessageBase : public Object {
  public:
    static const MessageType kType = 1;
    virtual MessageType type(void) const { return MessageBase::kType; }

  private:
    DISALLOW_COPY_AND_ASSIGN(MessageBase);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_MESSAGEBASE_H_
