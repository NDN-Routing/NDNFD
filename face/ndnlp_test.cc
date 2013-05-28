#include "ndnlp.h"
#include "message/interest.h"
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(FaceTest, Ndnlp) {
  uint8_t buf[2056];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, "\x01\xD2\xF2\xFA\x7F\xFD", 6);
  ASSERT_NE(nullptr, InterestMessage::Parse(buf, sizeof(buf)));
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, sizeof(buf));
  NetworkAddress netaddr;
  bool ok; std::list<Ptr<Buffer>> pkts; std::list<Ptr<Message>> msgs;
  
  Ptr<NdnlpWireProtocol> wp1 = NewTestElement<NdnlpWireProtocol>(900);
  ASSERT_TRUE(wp1->IsStateful());
  Ptr<WireProtocolState> s1 = wp1->CreateState(netaddr);
  std::tie(ok, pkts) = wp1->Encode(netaddr, s1, m1);
  ASSERT_TRUE(ok);
  ASSERT_EQ(3U, pkts.size());
  EXPECT_EQ(900U, pkts.front()->length());
  EXPECT_GE(900U, pkts.back()->length());
  
  std::tie(ok, msgs) = wp1->Decode(netaddr, s1, pkts.back());
  ASSERT_TRUE(ok);
  EXPECT_EQ(0U, msgs.size());
  pkts.pop_back();
  std::tie(ok, msgs) = wp1->Decode(netaddr, s1, pkts.front());
  ASSERT_TRUE(ok);
  EXPECT_EQ(0U, msgs.size());
  std::tie(ok, msgs) = wp1->Decode(netaddr, s1, pkts.front());//duplicate
  EXPECT_FALSE(ok);
  pkts.pop_front();
  std::tie(ok, msgs) = wp1->Decode(netaddr, s1, pkts.back());
  ASSERT_TRUE(ok);
  EXPECT_EQ(1U, msgs.size());

  Ptr<NdnlpWireProtocol> wp2 = NewTestElement<NdnlpWireProtocol>(9000);
  ASSERT_TRUE(wp2->IsStateful());
  Ptr<WireProtocolState> s2 = wp1->CreateState(netaddr);
  std::tie(ok, pkts) = wp2->Encode(netaddr, s2, m1);
  ASSERT_TRUE(ok);
  ASSERT_EQ(1U, pkts.size());
  EXPECT_GE(9000U, pkts.front()->length());
}

};//namespace ndnfd
