#ifndef NDNFD_MESSAGE_MESSAGEBASE_H_
#define NDNFD_MESSAGE_MESSAGEBASE_H_
#include "util/defs.h"
namespace ndnfd {

// MessageType indicates the type of a MessageBase subclass.
// Each subclass of MessageBase must have a unique MessageType.
typedef uint16_t MessageType;
const MessageType MessageType_none = 0;
MessageType MessageType_New(void);
#define MessageType_decl \
  static const MessageType kType; \
  virtual MessageType type(void) const { return kType; }
#define MessageType_def(cls) \
  const MessageType cls::kType = MessageType_New();

// MessageBase is the base class for most information moving between Elements.
class MessageBase : public Object {
 public:
  virtual ~MessageBase(void) {}
  MessageType_decl;

 protected:
  MessageBase(void) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageBase);
};

};//namespace ndnfd
#endif//NDNFD_MESSAGE_MESSAGEBASE_H_
