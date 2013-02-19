#include "ip.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(FaceTest, IpAddressParse) {
  Ptr<IpAddressVerifier> av = new IpAddressVerifier();
  bool ok; NetworkAddress addr;
  
  std::tie(ok, addr) = av->Parse("192.0.2.1:22");
  EXPECT_TRUE(ok);
  EXPECT_EQ(22, be16toh(reinterpret_cast<sockaddr_in*>(&addr.who)->sin_port));
  EXPECT_EQ(std::string("192.0.2.1:22"), av->ToString(addr));
  
  std::tie(ok, addr) = av->Parse("::1:80");
  EXPECT_TRUE(ok);
  EXPECT_EQ(80, be16toh(reinterpret_cast<sockaddr_in*>(&addr.who)->sin_port));
  EXPECT_EQ(std::string("[::1]:80"), av->ToString(addr));
}

};//namespace ndnfd
