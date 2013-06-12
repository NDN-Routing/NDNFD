#include "messagebase.h"
namespace ndnfd {

MessageType_def(MessageBase);

static uint32_t MessageType_last = 0;
MessageType MessageType_New(void) {
  return static_cast<MessageType>(++MessageType_last);
}

};//namespace ndnfd
