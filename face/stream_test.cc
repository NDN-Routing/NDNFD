#include "stream.h"
#include <fcntl.h>
#include "message/interest.h"
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

class StreamFaceTest_AddressVerifier : public AddressVerifier {
 public:
  StreamFaceTest_AddressVerifier(void) {}
  virtual ~StreamFaceTest_AddressVerifier(void) {}
  virtual std::string proto_name(void) const { return "StreamFaceTest_proto"; }
  virtual bool Check(const NetworkAddress& addr) const { return true; }
  virtual std::string ToString(const NetworkAddress& addr) const { return ""; };
 private:
  DISALLOW_COPY_AND_ASSIGN(StreamFaceTest_AddressVerifier);
};

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
  Ptr<AddressVerifier> av = new StreamFaceTest_AddressVerifier();
  Ptr<CcnbWireProtocol> ccnbwp = new CcnbWireProtocol(true);

  StreamFaceTest_MakeSocketPair(sockets);
  Ptr<StreamFace> f1 = NewTestElement<StreamFace>(sockets[1], false, netaddr, av, ccnbwp);
  EXPECT_EQ(FaceStatus::kUndecided, f1->status());
  ASSERT_TRUE(f1->CanSend());
  ASSERT_TRUE(f1->CanReceive());
  
  uint8_t buf[2056];
  memset(buf, 0, sizeof(buf));
  memcpy(buf, "\x01\xD2\xF2\xFA\x7F\xFD", 6);
  ASSERT_NE(nullptr, InterestMessage::Parse(buf, sizeof(buf)));
  Ptr<CcnbMessage> m1 = new CcnbMessage(buf, sizeof(buf));
  ASSERT_TRUE(m1->Verify());
  
  int sent = 0;
  while (!f1->send_blocked()) {
    ++sent;
    f1->Send(m1);
  }
  for (int i = 0; i < 10; ++i) {
    ++sent;
    f1->Send(m1);
  }
  // cannot test SetClosing() here: those cannot be received due to socketpair

  Ptr<StreamFace> f2 = NewTestElement<StreamFace>(sockets[0], false, netaddr, av, ccnbwp);
  int received = 0;
  f2->Receive = [&received] (Ptr<Message> msg) {
    ++received;
    CcnbMessage* m = static_cast<CcnbMessage*>(PeekPointer(msg));
    EXPECT_EQ(2056U, m->length());
    EXPECT_EQ(0xF2, m->msg()[2]);
  };
  while (received < sent) {
    TestGlobal()->pollmgr()->Poll(std::chrono::milliseconds(1000));
  }

  f1->SetClosing();
  EXPECT_EQ(FaceStatus::kFinalized, f1->status());
  EXPECT_FALSE(f1->CanSend());
  EXPECT_FALSE(f1->CanReceive());
}

// StreamListener is tested with TCP

};//namespace ndnfd
