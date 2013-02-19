#include "stream.h"
#include <fcntl.h>
#include "message/ccnb.h"
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

void StreamFaceTest_MakeSocketPair(int sockets[2]) {
  int res;
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
  ASSERT_NE(-1, res = fcntl(sockets[0], F_GETFL));
  ASSERT_EQ(0, fcntl(sockets[0], F_SETFL, res | O_NONBLOCK));
  ASSERT_NE(-1, res = fcntl(sockets[1], F_GETFL));
  ASSERT_EQ(0, fcntl(sockets[1], F_SETFL, res | O_NONBLOCK));
}

TEST(FaceTest, StreamFace) {
  int sockets[2];
  NetworkAddress netaddr;
  Ptr<CcnbWireProtocol> ccnbwp = new CcnbWireProtocol(true);

  TestGlobal->set_pollmgr(NewTestElement<PollMgr>());
  TestGlobal->set_facemgr(NewTestElement<FaceMgr>());

  StreamFaceTest_MakeSocketPair(sockets);
  Ptr<StreamFace> f1 = NewTestElement<StreamFace>(sockets[1], false, netaddr, ccnbwp);
  EXPECT_EQ(FaceStatus::kUndecided, f1->status());
  
  uint8_t buf[2054];
  memset(buf, 0, 2054);
  memcpy(buf, "\x4E\x64\x4C\xB2\x7F\xFD", 6);
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, 2054);
  ASSERT_TRUE(m1->Verify());
  
  int sent = 0;
  while (!f1->SendBlocked()) {
    ++sent;
    f1->Send(m1);
  }
  for (int i = 0; i < 10; ++i) {
    ++sent;
    f1->Send(m1);
  }
  // cannot test SetClosing() here: those cannot be received due to socketpair

  Ptr<StreamFace> f2 = NewTestElement<StreamFace>(sockets[0], false, netaddr, ccnbwp);
  int received = 0;
  f2->Receive = [&received](Ptr<Message> msg){
    ++received;
    CcnbMessage* m = dynamic_cast<CcnbMessage*>(PeekPointer(msg));
    EXPECT_EQ(2054U, m->length());
    EXPECT_EQ(0x4E, m->msg()[0]);
  };
  while (received < sent) {
    TestGlobal->pollmgr()->Poll(std::chrono::milliseconds(1000));
  }

  f1->SetClosing();
  EXPECT_EQ(FaceStatus::kClosed, f1->status());
}

// TODO unit test StreamListener in TCP

};//namespace ndnfd
