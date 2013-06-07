#include "ccnb.h"
#include "interest.h"
#include "contentobject.h"
#include "nack.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(MessageTest, CcnbMessage) {
  uint8_t buf[16];
  memset(buf, 0x80, sizeof(buf));
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

TEST(MessageTest, CcnbMessageParse) {
  Ptr<CcnbMessage> m1 = CcnbMessage::Parse(reinterpret_cast<const uint8_t*>("\x01\xD2\xF2\xFA\x8Dz\x00\x00\x00"), 9);
  EXPECT_NE(nullptr, m1);
  EXPECT_EQ(InterestMessage::kType, m1->type());
  
  Ptr<CcnbMessage> m2 = CcnbMessage::Parse(reinterpret_cast<const uint8_t*>("\x07\x82\x07\x8A\x9E""161\x00\x01\xD2\xF2\xFA\x8Dz\x00\x00\x00\x00"), 19);
  EXPECT_NE(nullptr, m2);
  EXPECT_EQ(NackMessage::kType, m2->type());
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

  Ptr<Buffer> pkt1 = new Buffer(9);
  memcpy(pkt1->mutable_data(), "\x01\xD2\xF2\xFA\x8Dz\x00\x00\x00", 9);
  std::tie(ok, msgs) = wp->Decode(netaddr, nullptr, pkt1);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(9U, static_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());
  
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
  memcpy(pkt->Put(21), "\x01\xD2\xF2\xFA\x8Dz\x00\x00\x00\x01\xD2\xF2\xFA\xB5\0\0\0\0\0\0\x00", 21);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(9U, static_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());
  
  pkt = state->GetReceiveBuffer();
  memcpy(pkt->Put(5), "\x00\x00\x01\xD2\xF2", 5);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(14U, static_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());
  
  pkt = state->GetReceiveBuffer();
  memcpy(pkt->Put(6), "\xFA\x8Dz\x00\x00\x00", 6);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  ASSERT_EQ(1U, msgs.size());
  EXPECT_EQ(9U, static_cast<CcnbMessage*>(PeekPointer(msgs.front()))->length());

  pkt = state->GetReceiveBuffer();
  memcpy(pkt->Put(6), "\x16\xEF\xEF\x00", 6);
  std::tie(ok, msgs) = wp->Decode(netaddr, state, pkt);
  EXPECT_FALSE(ok);
}

};//namespace ndnfd
