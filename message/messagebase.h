#ifndef NDNFD_MESSAGE_MESSAGEBASE_H_
#define NDNFD_MESSAGE_MESSAGEBASE_H_
#include "util/defs.h"
namespace ndnfd {

// MessageType indicates the type of a MessageBase subclass.
// Each subclass of MessageBase must have a unique MessageType.
typedef uint16_t MessageType;

// MessageBase is the base class for most information moving between Elements.
class MessageBase : public Object {
 public:
  static const MessageType kType = 1;
  virtual ~MessageBase(void) {}
  virtual MessageType type(void) const { return MessageBase::kType; }

 protected:
  MessageBase(void) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageBase);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_MESSAGEBASE_H_
