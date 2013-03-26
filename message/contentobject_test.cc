#include "contentobject.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, ContentObject) {
  Ptr<Name> n1 = Name::FromUri("/hello/world");
  std::basic_string<uint8_t> n1ccnb = n1->ToCcnb();

  ccn_charbuf* c1 = ccn_charbuf_create();
  ccnb_element_begin(c1, CCN_DTAG_ContentObject);
  ccnb_element_begin(c1, CCN_DTAG_Signature);
  ccnb_element_begin(c1, CCN_DTAG_DigestAlgorithm);
  ccn_charbuf_append_tt(c1, 3, CCN_UDATA);
  ccn_charbuf_append_string(c1, "NOP");
  ccnb_element_end(c1);//</DigestAlgorithm>
  ccnb_append_tagged_blob(c1, CCN_DTAG_SignatureBits, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
  ccnb_element_end(c1);//</Signature>
  ccn_charbuf_append(c1, n1ccnb.data(), n1ccnb.size());
  ccnb_element_begin(c1, CCN_DTAG_SignedInfo);
  ccnb_append_tagged_blob(c1, CCN_DTAG_PublisherPublicKeyDigest, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
  ccnb_append_tagged_binary_number(c1, CCN_DTAG_Timestamp, 0x10000);
  ccnb_element_end(c1);//</SignedInfo>
  ccnb_append_tagged_blob(c1, CCN_DTAG_Content, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26);
  ccnb_element_end(c1);//</ContentObject>

  Ptr<ContentObjectMessage> co1 = ContentObjectMessage::Parse(c1->buf, c1->length);
  ASSERT_NE(nullptr, co1);
  
  Ptr<Name> n1a = co1->name();
  EXPECT_TRUE(n1->Equals(n1a));
  
  const uint8_t* payload; size_t payload_size;
  std::tie(payload, payload_size) = co1->payload();
  ASSERT_NE(nullptr, payload);
  EXPECT_EQ(26U, payload_size);
  EXPECT_EQ('A', static_cast<char>(payload[0]));
  
  ccn_charbuf_destroy(&c1);
}

};//namespace ndnfd
