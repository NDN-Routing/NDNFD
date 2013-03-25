#include "simclock.h"
#include "gtest/gtest.h"
namespace ndnfd {

void SimClockTest_SaveNow(SimClock::time_point* now) {
  *now = SimClock::now();
}

TEST(SimTest, SimClock) {
  EXPECT_EXIT({
    SimClock::time_point t0 = SimClock::now();
    SimClock::time_point t1;
    SimClock::time_point t2;
    ns3::Simulator::Schedule(ns3::MilliSeconds(1000), &SimClockTest_SaveNow, &t1);
    ns3::Simulator::Schedule(ns3::MilliSeconds(1500), &SimClockTest_SaveNow, &t2);
    ns3::Simulator::Run();
    if (std::chrono::time_point_cast<std::chrono::milliseconds>(t0).time_since_epoch().count() != 0) exit(1);
    if (std::chrono::time_point_cast<std::chrono::milliseconds>(t1).time_since_epoch().count() != 1000) exit(2);
    if (std::chrono::time_point_cast<std::chrono::milliseconds>(t2).time_since_epoch().count() != 1500) exit(3);
    exit(0);
  }, ::testing::ExitedWithCode(0), "");
}

};//namespace ndnfd
