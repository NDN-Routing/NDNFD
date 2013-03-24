#include "name.h"
extern "C" {
#include <ccn/uri.h>
}
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, Name) {
  ccn_charbuf* c1 = ccn_charbuf_create();
  EXPECT_EQ(12, ccn_name_from_uri(c1, "/hello/world"));
  Ptr<Name> n1 = Name::FromCcnb(c1->buf, c1->length);
  ccn_charbuf_destroy(&c1);
  ASSERT_NE(nullptr, n1);
  EXPECT_EQ(2, n1->n_comps());
  EXPECT_EQ(0, n1->comps()[0].compare(reinterpret_cast<const uint8_t*>("hello")));
  
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
