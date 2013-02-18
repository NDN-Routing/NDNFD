#include "outgoing.h"
#include "gtest/gtest.h"
namespace ndnfd {

class OutgoingTestMessage : public Message {
 public:
  OutgoingTestMessage(void) {}
  ~OutgoingTestMessage(void) {}
  virtual MessageType type(void) const { return 5; }
 private:
  DISALLOW_COPY_AND_ASSIGN(OutgoingTestMessage);
};

TEST(MessageTest, Outgoing) {
  Ptr<OutgoingTestMessage> m1 = new OutgoingTestMessage();

  Ptr<Outgoing> o1 = new Outgoing(m1, 20);
  EXPECT_EQ(m1, o1->message());
  EXPECT_EQ(5, o1->type());
  EXPECT_EQ(20, o1->outgoing_face());
  EXPECT_TRUE(o1->Sending());
  EXPECT_FALSE(o1->Sending());

  Ptr<Outgoing> o2 = new Outgoing(m1, 25);
  EXPECT_TRUE(o2->Cancel());
  EXPECT_TRUE(o2->Cancel());
  EXPECT_FALSE(o2->Sending());

  Ptr<Outgoing> o3 = new Outgoing(m1, 30);
  EXPECT_TRUE(o3->Sending());
  EXPECT_FALSE(o3->Cancel());
}

};//namespace ndnfd
