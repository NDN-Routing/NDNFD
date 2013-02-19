#include "ccnb.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, CcnbMessage) {
  uint8_t buf[16];
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, 16);
  EXPECT_EQ(buf, m1->msg());
  EXPECT_EQ(16U, m1->length());
  EXPECT_FALSE(m1->Verify());
  
  memcpy(buf, "\x4E\x64\x4C\xB2\x00", 5);
  Ptr<CcnbMessage> m2 = new CcnbMessage(buf, 5);
  EXPECT_EQ(buf, m2->msg());
  EXPECT_EQ(5U, m2->length());
  EXPECT_TRUE(m2->Verify());
}

TEST(MessageTest, CcnbWireProtoEncode) {
  NetworkAddress netaddr;
  uint8_t buf[16];
  memcpy(buf, "\x4E\x64\x4C\xB2\x00", 5);
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, 5);

  Ptr<CcnbWireProtocol> wp = new CcnbWireProtocol(false);
  bool ok; std::list<Ptr<Buffer>> pkts;
  std::tie(ok, pkts) = wp->Encode(netaddr, nullptr, m1);
  EXPECT_TRUE(ok);
  ASSERT_EQ(1U, pkts.size());
  EXPECT_EQ(5U, pkts.front()->length());
}

TEST(MessageTest, CcnbWireProtoDecodeDgram) {
  NetworkAddress netaddr;

  Ptr<CcnbWireProtocol> wp = new CcnbWireProtocol(false);
  ASSERT_FALSE(wp->IsStateful());
  bool ok; std::list<Ptr<Message>> msgs;

  Ptr<Buffer> pkt1 = new Buffer(5);
  memcpy(pkt1->mutable_data(), "\x4E\x64\x4C\xB2\x00", 5);
  std::tie(ok, msgs) = wp->Decode(netaddr, nullptr, pkt1);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(5U, dynamic_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());
  
  pkt1->Put(1);
  std::tie(ok, msgs) = wp->Decode(netaddr, nullptr, pkt1);
  EXPECT_FALSE(ok);
  
  pkt1->mutable_data()[0] = '\x00';
  std::tie(ok, msgs) = wp->Decode(netaddr, nullptr, pkt1);
  EXPECT_FALSE(ok);
}

TEST(MessageTest, CcnbWireProtoDecodeStream) {
  NetworkAddress netaddr;

  Ptr<CcnbWireProtocol> wp = new CcnbWireProtocol(true);
  ASSERT_TRUE(wp->IsStateful());
  Ptr<WireProtocolState> state = wp->CreateState(netaddr);
  bool ok; std::list<Ptr<Message>> msgs;
  Ptr<Buffer> pkt;

  pkt = state->GetReceiveBuffer();
  memcpy(pkt->Put(21), "\x4E\x64\x4C\xB2\x00\x4E\x64\x4C\xBA\x4E\x64\x4C\xC2\xB5\0\0\0\0\0\0\x00", 21);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(5U, dynamic_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());
  
  pkt = state->GetReceiveBuffer();
  memcpy(pkt->Put(4), "\0\x4E\x64\x4C", 4);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(17U, dynamic_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());
  
  pkt = state->GetReceiveBuffer();
  memcpy(pkt->Put(2), "\xCA\0", 2);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(5U, dynamic_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());

  pkt = state->GetReceiveBuffer();
  memcpy(pkt->Put(6), "\x16\xEF\xEF\x00", 6);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  EXPECT_FALSE(ok);
}

};//namespace ndnfd
