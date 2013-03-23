#include "addrtable.h"
#include "ip.h"
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(FaceTest, FaceAddressTable) {
  bool ok; NetworkAddress a1,a2;
  std::tie(ok, a1) = IpAddressVerifier::Parse("192.0.2.1:80"); ASSERT_TRUE(ok);
  std::tie(ok, a2) = IpAddressVerifier::Parse("192.0.2.2:80"); ASSERT_TRUE(ok);
  
  Ptr<IpAddressVerifier> av = new IpAddressVerifier();
  Ptr<FaceAddressTable> fat = NewTestElement<FaceAddressTable>(av);
  FaceId conflict;
  
  EXPECT_EQ(FaceId_none, fat->Find(a1));
  std::tie(ok, conflict) = fat->Add(a1, 1);
  EXPECT_TRUE(ok);
  EXPECT_EQ(FaceId_none, conflict);
  EXPECT_EQ(1, fat->Find(a1));
  std::tie(ok, conflict) = fat->Add(a1, 2);
  EXPECT_FALSE(ok);
  EXPECT_EQ(1, conflict);
  
  std::tie(ok, conflict) = fat->Add(a2, 2);
  EXPECT_TRUE(ok);
  EXPECT_EQ(FaceId_none, conflict);
  EXPECT_EQ(1, fat->Find(a1));
  EXPECT_EQ(2, fat->Find(a2));
  
  fat->Remove(1);
  EXPECT_EQ(FaceId_none, fat->Find(a1));
  EXPECT_EQ(2, fat->Find(a2));
}

};//namespace ndnfd
