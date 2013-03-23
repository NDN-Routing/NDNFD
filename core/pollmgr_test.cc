#include "pollmgr.h"
#include "gtest/gtest.h"
#include "core/element_testh.h"
#include <sys/socket.h>
namespace ndnfd {

class PollMgrTestClient : public IPollClient {
 public:
  std::vector<std::tuple<int,short>> log_;
  virtual void PollCallback(int fd, short revents) {
    this->log_.push_back(std::make_tuple(fd, revents));
  }
};

TEST(CoreTest, PollMgr) {
  int sockets[2]; int fd; short revents;
  ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
  
  Ptr<PollMgr> pm = NewTestElement<PollMgr>();
  PollMgrTestClient client;
  
  pm->Add(&client, sockets[0], POLLOUT);
  client.log_.clear();
  pm->Poll(std::chrono::milliseconds(100));
  EXPECT_EQ(1U, client.log_.size());
  std::tie(fd, revents) = client.log_.front();
  EXPECT_EQ(sockets[0], fd);
  EXPECT_NE(0, revents & POLLOUT);
  EXPECT_EQ(0, revents & POLLIN);
  
  pm->Remove(&client, sockets[0], POLLOUT);
  client.log_.clear();
  pm->Poll(std::chrono::milliseconds(100));
  EXPECT_EQ(0U, client.log_.size());

  pm->Add(&client, sockets[0], POLLOUT);
  pm->Add(&client, sockets[0], POLLIN);
  client.log_.clear();
  pm->Poll(std::chrono::milliseconds(100));
  EXPECT_EQ(1U, client.log_.size());
  std::tie(fd, revents) = client.log_.front();
  EXPECT_EQ(sockets[0], fd);
  EXPECT_NE(0, revents & POLLOUT);
  EXPECT_EQ(0, revents & POLLIN);
  
  ASSERT_EQ(2, write(sockets[1], "..", 2));
  client.log_.clear();
  pm->Poll(std::chrono::milliseconds(100));
  EXPECT_EQ(1U, client.log_.size());
  std::tie(fd, revents) = client.log_.front();
  EXPECT_EQ(sockets[0], fd);
  EXPECT_NE(0, revents & POLLOUT);
  EXPECT_NE(0, revents & POLLIN);
  
  pm->RemoveAll(&client);
  client.log_.clear();
  pm->Poll(std::chrono::milliseconds(100));
  EXPECT_EQ(0U, client.log_.size());
  
  pm->Add(&client, sockets[1], POLLIN);
  close(sockets[0]);
  client.log_.clear();
  pm->Poll(std::chrono::milliseconds(100));
  EXPECT_EQ(1U, client.log_.size());
  std::tie(fd, revents) = client.log_.front();
  EXPECT_EQ(sockets[1], fd);
  EXPECT_NE(0, revents & POLLHUP);

  pm->Add(&client, sockets[0], POLLOUT);
  client.log_.clear();
  pm->Poll(std::chrono::milliseconds(100));
  EXPECT_EQ(2U, client.log_.size());
  
  close(sockets[1]);
}

};//namespace ndnfd
