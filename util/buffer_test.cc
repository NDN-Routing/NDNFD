#include "buffer.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(UtilTest, Buffer) {
  Ptr<Buffer> b1 = new Buffer(40, 8, 8);
  EXPECT_EQ(40U, b1->length());
  EXPECT_EQ(40U, b1->end_data() - b1->data());
  b1->mutable_data()[0] = 0xBE;
  EXPECT_EQ(0xBE, b1->data()[0]);
  
  b1->Put(2)[0] = 0xAA;
  EXPECT_EQ(42U, b1->length());
  EXPECT_EQ(0xBE, b1->data()[0]);
  EXPECT_EQ(0xAA, b1->data()[40]);
  b1->Take(6);
  EXPECT_EQ(36U, b1->length());
  EXPECT_EQ(0xBE, b1->data()[0]);
  b1->Put(44);
  EXPECT_EQ(80U, b1->length());
  EXPECT_EQ(0xBE, b1->data()[0]);
  b1->mutable_data()[79] = 0x0D;
  
  b1->Pull(78);
  EXPECT_EQ(2U, b1->length());
  EXPECT_EQ(0x0D, b1->data()[1]);
  b1->Push(512)[0] = 0xC1;
  EXPECT_EQ(514U, b1->length());
  EXPECT_EQ(0xC1, b1->data()[0]);
  EXPECT_EQ(0x0D, b1->data()[513]);
  
  Ptr<Buffer> b2 = b1->AsBuffer(false);
  EXPECT_EQ(514U, b2->length());
  EXPECT_EQ(0xC1, b2->data()[0]);
  EXPECT_EQ(0x0D, b2->data()[513]);

  Ptr<Buffer> b3 = b1->AsBuffer(true);
  EXPECT_EQ(514U, b3->length());
  EXPECT_EQ(0xC1, b3->data()[0]);
  EXPECT_EQ(0x0D, b3->data()[513]);
  EXPECT_NE(b1->data(), b3->data());
}

};//namespace ndnfd
