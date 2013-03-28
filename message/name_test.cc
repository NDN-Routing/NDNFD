#include "name.h"
extern "C" {
#include <ccn/ccn.h>
#include <ccn/uri.h>
}
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, Name) {
  ccn_charbuf* c1 = ccn_charbuf_create();
  EXPECT_EQ(12, ccn_name_from_uri(c1, "/hello/world"));
  Ptr<Name> n1 = Name::FromCcnb(c1->buf, c1->length);
  ASSERT_NE(nullptr, n1);
  EXPECT_EQ(2, n1->n_comps());
  EXPECT_EQ(0, n1->comps()[0].compare(reinterpret_cast<const uint8_t*>("hello")));

  ccn_indexbuf* comps2 = ccn_indexbuf_create();
  std::basic_string<uint8_t> c2 = n1->ToCcnb(true, comps2);
  ASSERT_EQ(c1->length, c2.size());
  EXPECT_EQ(0, memcmp(c1->buf, c2.data(), c1->length));
  ASSERT_EQ(n1->n_comps() + 1U, comps2->n);
  ccn_indexbuf* comps1 = ccn_indexbuf_create();
  int ncomps1 = ccn_name_split(c1, comps1);
  ASSERT_EQ(n1->n_comps(), static_cast<uint32_t>(ncomps1));
  ASSERT_EQ(n1->n_comps() + 1U, comps1->n);
  for (uint32_t i = 0; i <= n1->n_comps(); ++i) {
    EXPECT_EQ(comps1->buf[i], comps2->buf[i]);
  }
  ccn_indexbuf_destroy(&comps2);
  ccn_indexbuf_destroy(&comps1);
  ccn_charbuf_destroy(&c1);
  
  Ptr<Name> n2 = Name::FromUri("/hello/world/earth");
  ASSERT_NE(nullptr, n2);
  EXPECT_EQ(3, n2->n_comps());
  EXPECT_TRUE(n1->IsPrefixOf(n2));
  EXPECT_FALSE(n1->Equals(n2));
  EXPECT_FALSE(n2->IsPrefixOf(n1));
  
  EXPECT_TRUE(n2->GetPrefix(0)->Equals(n1->GetPrefix(0)));
  EXPECT_TRUE(n2->GetPrefix(2)->Equals(n1));
  EXPECT_TRUE(n2->Equals(n1->Append(reinterpret_cast<const uint8_t*>("earth"))));
}

};//namespace ndnfd
