#include "messagebase.h"
#include "gtest/gtest.h"
namespace ndnfd {

class MessageTypeTestMessage : public MessageBase {
 public:
  MessageTypeTestMessage(void) {}
  MessageType_decl;

 private:
  DISALLOW_COPY_AND_ASSIGN(MessageTypeTestMessage);
};
MessageType_def(MessageTypeTestMessage);

TEST(MessageTest, MessageType) {
  EXPECT_NE(MessageBase::kType, MessageTypeTestMessage::kType);
}

};//namespace ndnfd
