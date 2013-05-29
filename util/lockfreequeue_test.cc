#include "lockfreequeue.h"

#if !defined(_GLIBCXX_USE_NANOSLEEP) && (__GNUC__*1000+__GNUC_MINOR__)<=4007
#define HACK_GCCBUG52680
#define _GLIBCXX_USE_NANOSLEEP
#endif
#include <thread>
#ifdef HACK_GCCBUG52680
#undef HACK_GCCBUG52680
#undef _GLIBCXX_USE_NANOSLEEP
#endif

#include "gtest/gtest.h"
namespace ndnfd {

TEST(UtilTest, LockFreeQueue) {
  LockFreeQueue<int> queue;
  std::queue<int> received;
  int pop_on_empty = 0;
  std::thread producer([&queue]{
    for (int i = 0; i < 1024; ++i) {
      queue.push(i);
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  });
  std::thread consumer([&queue,&received,&pop_on_empty]{
    bool ok; int value;
    do {
      std::tie(ok, value) = queue.pop();
      if (ok) {
        received.push(value);
      } else {
        ++pop_on_empty;
      }
    } while (received.size() < 1024);
  });
  producer.join();
  consumer.join();
  ASSERT_EQ(1024U, received.size());
  for (int i = 0; i < 1024; ++i) {
    EXPECT_EQ(i, received.front());
    received.pop();
  }
  printf("pop_on_empty=%d\n", pop_on_empty);
}

};//namespace ndnfd
