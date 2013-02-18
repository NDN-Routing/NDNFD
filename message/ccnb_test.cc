#include "ccnb.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, CcnbMessage) {
  uint8_t buf[16];
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, 16);
  EXPECT_EQ(buf, m1->msg());
  EXPECT_EQ(16U, m1->length());
  EXPECT_FALSE(m1->Verify());
  
  ::memcpy(buf, "\x4E\x64\x4C\xB2\x00", 5);
  Ptr<CcnbMessage> m2 = new CcnbMessage(buf, 5);
  EXPECT_EQ(buf, m2->msg());
  EXPECT_EQ(5U, m2->length());
  EXPECT_TRUE(m2->Verify());
}

};//namespace ndnfd
