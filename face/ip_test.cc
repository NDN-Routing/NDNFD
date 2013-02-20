#include "ip.h"
#include "message/ccnb.h"
#include "core/element_testh.h"
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

TEST(FaceTest, Tcp) {
  bool ok; NetworkAddress addr;
  Ptr<IpAddressVerifier> av = new IpAddressVerifier();
  std::tie(ok, addr) = av->Parse("127.0.0.1:13602");
  ASSERT_TRUE(ok);
  Ptr<CcnbWireProtocol> ccnbwp = new CcnbWireProtocol(true);
  
  TestGlobal->set_pollmgr(NewTestElement<PollMgr>());
  TestGlobal->set_facemgr(NewTestElement<FaceMgr>());

  uint8_t buf[16];
  memcpy(buf, "\x4E\x64\x4C\xB2\x00", 5);
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, 5);

  Ptr<TcpFaceFactory> factory = NewTestElement<TcpFaceFactory>(ccnbwp);
  Ptr<StreamListener> listener = factory->Listen(addr);
  
  int accepted = 0;
  ASSERT_TRUE(listener->CanAccept());
  listener->Accept = [&accepted,&m1] (Ptr<Face> face) {
    ++accepted;
    EXPECT_TRUE(face->CanSend());
    EXPECT_TRUE(face->CanReceive());
    face->Send(m1);
    dynamic_cast<StreamFace*>(PeekPointer(face))->SetClosing();
  };
  
  int received = 0;
  Ptr<StreamFace> clients[10];
  for (int i = 0; i < 10; ++i) {
    clients[i] = factory->Connect(addr);
    EXPECT_TRUE(clients[i]->CanSend());
    EXPECT_TRUE(clients[i]->CanReceive());
    clients[i]->Receive = [&received] (Ptr<Message> msg) {
      ++received;
    };
  }

  while (accepted < 10 || received < 10) {
    TestGlobal->pollmgr()->Poll(std::chrono::milliseconds(1000));
  }
  listener->Close();
}

};//namespace ndnfd
