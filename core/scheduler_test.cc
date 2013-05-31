#include "scheduler.h"
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

void SchedulerTestGetTime(const ccn_gettime* self, ccn_timeval* result) {
  auto now = std::chrono::system_clock::now();//XXX should be steady_clock, but gcc 4.6 doesn't have it
  int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
  result->s = now_us / 1000000;
  result->micros = now_us % 1000000;
}

TEST(CoreTest, Scheduler) {
  ccn_gettime gt = { "", &SchedulerTestGetTime, 1000000, nullptr };
  Ptr<Scheduler> s = NewTestElement<Scheduler>(ccn_schedule_create(nullptr, &gt));
  int r1 = 0, r2 = 0;
  SchedulerEvent evt1 = s->Schedule(std::chrono::microseconds(10), [&r1]()->std::chrono::microseconds{
    ++r1;
    if (r1 == 1) return std::chrono::microseconds(20);
    return Scheduler::kNoMore;
  });
  ASSERT_NE(nullptr, evt1);
  SchedulerEvent evt2 = s->Schedule(std::chrono::microseconds(100), [&r2]()->std::chrono::microseconds{
    ++r2;
    return Scheduler::kNoMore;
  });
  ASSERT_NE(nullptr, evt2);
  s->Cancel(evt2);
  std::chrono::microseconds next_evt(0xFFFF);
  do {
    next_evt = s->Run();
  } while (next_evt != Scheduler::kNoMore);
  EXPECT_EQ(2, r1);
  EXPECT_EQ(0, r2);
}

};//namespace ndnfd
