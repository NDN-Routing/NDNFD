#include "interest.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, Interest) {
  Ptr<Name> n1 = Name::FromUri("/hello/world");
  std::basic_string<uint8_t> n1ccnb = n1->ToCcnb();

  ccn_charbuf* c1 = ccn_charbuf_create();
  ccnb_element_begin(c1, CCN_DTAG_Interest);
  ccn_charbuf_append(c1, n1ccnb.data(), n1ccnb.size());
  ccnb_element_end(c1);//</Interest>

  Ptr<InterestMessage> i1 = InterestMessage::Parse(c1->buf, c1->length);
  ASSERT_NE(nullptr, i1);
  
  Ptr<Name> n1a = i1->name();
  EXPECT_TRUE(n1->Equals(n1a));
  
  ccn_charbuf_destroy(&c1);
}

};//namespace ndnfd
