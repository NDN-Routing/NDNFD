#include "nameprefix_table.h"
extern "C" {
#include <ccn/hashtb.h>
}
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(CoreTest, NamePrefixTable) {
  TestGlobal()->ccndh()->nameprefix_tab = hashtb_create(sizeof(nameprefix_entry), nullptr);
  Ptr<NamePrefixTable> npt = TestGlobal()->npt();
  
  Ptr<Name> n1 = Name::FromUri("/hello/world");
  EXPECT_EQ(nullptr, npt->Get(n1));
  Ptr<NamePrefixEntry> npe1 = npt->Seek(n1);
  ASSERT_NE(nullptr, npe1);
  EXPECT_TRUE(n1->Equals(npe1->name()));
  Ptr<NamePrefixEntry> npe2 = npt->Get(n1);
  ASSERT_NE(nullptr, npe2);
  EXPECT_TRUE(n1->Equals(npe2->name()));
  npe2 = npe1->FibNode();
  EXPECT_EQ(nullptr, npe2);
  npe2 = npe1->Parent();//"/hello"
  ASSERT_NE(nullptr, npe2);
  npe2 = npe2->Parent();//"/"
  ASSERT_NE(nullptr, npe2);
  npe2 = npe2->Parent();
  EXPECT_EQ(nullptr, npe2);
  
  EXPECT_EQ(nullptr, npe1->GetForwarding(1));
  Ptr<ForwardingEntry> f1 = npe1->SeekForwarding(1);
  ASSERT_NE(nullptr, f1);
  EXPECT_TRUE(n1->Equals(f1->name()));
  EXPECT_TRUE(n1->Equals(f1->npe()->name()));
  Ptr<ForwardingEntry> f2 = npe1->GetForwarding(1);
  ASSERT_NE(nullptr, f2);
  EXPECT_TRUE(n1->Equals(f2->name()));
  npe2 = npe1->FibNode();
  ASSERT_NE(nullptr, npe2);
  EXPECT_TRUE(n1->Equals(npe2->name()));
  
  Ptr<Name> n3 = n1->Append(reinterpret_cast<const uint8_t*>("p"));
  Ptr<Name> n4 = n3->Append(reinterpret_cast<const uint8_t*>("q"));
  EXPECT_EQ(nullptr, npt->Get(n4));
  Ptr<NamePrefixEntry> npe4 = npt->Seek(n4);
  ASSERT_NE(nullptr, npe4);
  EXPECT_TRUE(n4->Equals(npe4->name()));
  npe2 = npe4->FibNode();
  ASSERT_NE(nullptr, npe2);
  EXPECT_TRUE(n1->Equals(npe2->name()));
  npe2 = npe4->Parent();
  ASSERT_NE(nullptr, npe2);
  EXPECT_TRUE(n3->Equals(npe2->name()));
  
  hashtb_destroy(&(TestGlobal()->ccndh()->nameprefix_tab));
  TestGlobal()->ccndh()->nameprefix_tab = hashtb_create(sizeof(nameprefix_entry), nullptr);
}

};//namespace ndnfd
