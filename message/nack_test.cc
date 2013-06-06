#include "nack.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, Nack) {
  ccn_charbuf* c1 = ccn_charbuf_create();
  ccnb_element_begin(c1, CCN_DTAG_StatusResponse);
  ccnb_tagged_putf(c1, CCN_DTAG_StatusCode, "%d", static_cast<int>(NackCode::kCongestion));
  ccn_charbuf_append(c1, "\x01\xD2\xF2\xFA\x8Dz\x00\x00\x00", 9);
  ccnb_element_end(c1);//</StatusResponse>

  Ptr<NackMessage> n1 = NackMessage::Parse(c1->buf, c1->length);
  ASSERT_NE(nullptr, n1);
  
  EXPECT_EQ(NackCode::kCongestion, n1->code());
  EXPECT_TRUE(n1->interest()->name()->Equals(Name::FromUri("/z")));
  
  ccn_charbuf_destroy(&c1);
}

};//namespace ndnfd
