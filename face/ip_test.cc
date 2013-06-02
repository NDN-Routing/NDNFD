#include "ip.h"
#include "util/endian.h"
#include "message/interest.h"
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(FaceTest, IpAddressParse) {
  Ptr<IpAddressVerifier> av = new IpAddressVerifier("tcp");
  bool ok; NetworkAddress addr;
  
  std::tie(ok, addr) = av->Parse("192.0.2.1:22");
  EXPECT_TRUE(ok);
  EXPECT_EQ(22, be16toh(reinterpret_cast<sockaddr_in*>(&addr.who)->sin_port));
  EXPECT_EQ(std::string("192.0.2.1:22"), av->ToString(addr));
  EXPECT_FALSE(av->IsLocal(addr));
  EXPECT_FALSE(av->IsMcast(addr));
  
  std::tie(ok, addr) = av->Parse("224.0.0.0:22");
  EXPECT_TRUE(av->IsMcast(addr));
  
  std::tie(ok, addr) = av->Parse("::1:80");
  EXPECT_TRUE(ok);
  EXPECT_EQ(80, be16toh(reinterpret_cast<sockaddr_in*>(&addr.who)->sin_port));
  EXPECT_EQ(std::string("[::1]:80"), av->ToString(addr));
  EXPECT_TRUE(av->IsLocal(addr));
  EXPECT_FALSE(av->IsMcast(addr));
  
  std::tie(ok, addr) = av->Parse("ff00::1:80");
  EXPECT_TRUE(av->IsMcast(addr));
}

TEST(FaceTest, Udp) {
  bool ok; NetworkAddress addr1, addr2, addr3;
  std::tie(ok, addr1) = IpAddressVerifier::Parse("127.0.0.1:23089");
  ASSERT_TRUE(ok);
  std::tie(ok, addr2) = IpAddressVerifier::Parse("127.0.0.1:28075");
  ASSERT_TRUE(ok);
  std::tie(ok, addr3) = IpAddressVerifier::Parse("127.0.0.1:10971");
  ASSERT_TRUE(ok);
  Ptr<CcnbWireProtocol> ccnbwp = new CcnbWireProtocol(false);
  
  uint8_t buf[9];
  memcpy(buf, "\x01\xD2\xF2\xFA\x8Dz\x00\x00\x00", sizeof(buf));
  ASSERT_NE(nullptr, InterestMessage::Parse(buf, sizeof(buf)));
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, sizeof(buf));

  Ptr<UdpFaceFactory> factory = NewTestElement<UdpFaceFactory>(ccnbwp);
  Ptr<DgramChannel> ch1 = factory->Channel(addr1);
  ASSERT_NE(nullptr, ch1);
  Ptr<DgramChannel> ch2 = factory->Channel(addr2);
  ASSERT_NE(nullptr, ch2);
  Ptr<DgramChannel> ch3 = factory->Channel(addr3);
  ASSERT_NE(nullptr, ch3);
  
  int r10 = 0;
  ch1->GetFallbackFace()->Receive = [&r10] (Ptr<Message> msg) { ++r10; };
  
  Ptr<DgramFace> f21 = ch2->GetFace(addr1);
  for (int i = 0; i < 10; ++i) {
    f21->Send(m1);
    if (r10 > 0) break;
    TestGlobal()->pollmgr()->Poll(std::chrono::milliseconds(100));
  }
  EXPECT_NE(0, r10);
  r10 = 0;
  for (int i = 0; i < 5; ++i) {
    r10 = 0;
    TestGlobal()->pollmgr()->Poll(std::chrono::milliseconds(100));
    if (r10 == 0) break;
  }
  r10 = 0;
  
  FaceDescription f21d = f21->GetDescription();
  EXPECT_EQ("UDP", f21d.proto_);
  EXPECT_EQ("127.0.0.1:23089", f21d.peer_);
  
  Ptr<DgramFace> f12 = ch1->GetFace(addr2);
  int r12 = 0;
  f12->Receive = [&r12] (Ptr<Message> msg) { ++r12; };
  for (int i = 0; i < 10; ++i) {
    f21->Send(m1);
    if (r12 > 0) break;
    TestGlobal()->pollmgr()->Poll(std::chrono::milliseconds(100));
  }
  EXPECT_NE(0, r12);
  EXPECT_EQ(0, r10);
  
  ch1->Close();
  ch2->Close();
  ch3->Close();
}

TEST(FaceTest, Tcp) {
  bool ok; NetworkAddress addr;
  std::tie(ok, addr) = IpAddressVerifier::Parse("127.0.0.1:13602");
  ASSERT_TRUE(ok);
  Ptr<CcnbWireProtocol> ccnbwp = new CcnbWireProtocol(true);
  
  uint8_t buf[9];
  memcpy(buf, "\x01\xD2\xF2\xFA\x8Dz\x00\x00\x00", sizeof(buf));
  ASSERT_NE(nullptr, InterestMessage::Parse(buf, sizeof(buf)));
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, sizeof(buf));

  Ptr<TcpFaceFactory> factory = NewTestElement<TcpFaceFactory>(ccnbwp);
  Ptr<StreamListener> listener = factory->Listen(addr);
  ASSERT_NE(nullptr, listener);
  
  int accepted = 0;
  ASSERT_TRUE(listener->CanAccept());
  listener->Accept = [&accepted,&m1] (Ptr<Face> face) {
    ++accepted;
    EXPECT_TRUE(face->CanSend());
    EXPECT_TRUE(face->CanReceive());
    face->Send(m1);
    static_cast<StreamFace*>(PeekPointer(face))->SetClosing();
  };
  
  int received = 0;
  EXPECT_EQ(nullptr, factory->FindFace(addr));
  Ptr<StreamFace> client = factory->Connect(addr);
  EXPECT_TRUE(client->CanSend());
  EXPECT_TRUE(client->CanReceive());
  client->Receive = [&received] (Ptr<Message> msg) {
    ++received;
  };
  EXPECT_EQ(client, factory->FindFace(addr));
  Ptr<StreamFace> client2 = factory->Connect(addr);
  EXPECT_EQ(client, client2);

  while (accepted < 1 || received < 1) {
    TestGlobal()->pollmgr()->Poll(std::chrono::milliseconds(100));
  }
  listener->Close();
}

};//namespace ndnfd
