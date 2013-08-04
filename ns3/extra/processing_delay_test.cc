#include "processing_delay.h"
#include <ns3/core-module.h>
#include "gtest/gtest.h"
namespace ns3 {

static Ptr<ProcessingDelay> pd;
static std::vector<std::tuple<uint64_t,Time>> ProcessingDelayTest_completed;
static std::vector<uint64_t> ProcessingDelayTest_dropped;
static Time ProcessingDelayTest_expectCanStartImmediately_fail_time;

void ProcessingDelayTest_complete(ProcessingDelay::Job job) {
  Ptr<const UintegerValue> val = static_cast<const UintegerValue*>(&job);
  uint64_t n = val->Get();
  ProcessingDelayTest_completed.emplace_back(n, Now());
}

void ProcessingDelayTest_drop(ProcessingDelay::Job job) {
  Ptr<const UintegerValue> val = static_cast<const UintegerValue*>(&job);
  uint64_t n = val->Get();
  ProcessingDelayTest_dropped.emplace_back(n);
}

void ProcessingDelayTest_expectCanStartImmediately(bool expected) {
  if (pd->CanStartImmediately() != expected) {
    ProcessingDelayTest_expectCanStartImmediately_fail_time = Simulator::Now();
  }
}

TEST(SimTest, ProcessingDelay) {
  ProcessingDelayTest_completed.clear();
  ProcessingDelayTest_dropped.clear();
  ProcessingDelayTest_expectCanStartImmediately_fail_time = MilliSeconds(-1);
  
  pd = CreateObject<ProcessingDelay>();
  pd->SetAttribute("NSlots", UintegerValue(2));
  pd->SetAttribute("QueueCapacity", UintegerValue(2));
  pd->SetAttribute("ProcessTime", StringValue("10ms"));
  pd->SetCompleteCallback(MakeCallback(&ProcessingDelayTest_complete));
  pd->TraceConnectWithoutContext("Drop", MakeCallback(&ProcessingDelayTest_drop));
  
  Simulator::Schedule(MilliSeconds(1), &ProcessingDelay::SubmitJob, pd, UintegerValue(1));
  Simulator::Schedule(MilliSeconds(4), &ProcessingDelayTest_expectCanStartImmediately, true);
  Simulator::Schedule(MilliSeconds(5), &ProcessingDelay::SubmitJob, pd, UintegerValue(2));
  Simulator::Schedule(MilliSeconds(6), &ProcessingDelayTest_expectCanStartImmediately, false);
  Simulator::Schedule(MilliSeconds(7), &ProcessingDelay::SubmitJob, pd, UintegerValue(3));
  Simulator::Schedule(MilliSeconds(8), &ProcessingDelay::SubmitJob, pd, UintegerValue(4));
  Simulator::Schedule(MilliSeconds(9), &ProcessingDelay::SubmitJob, pd, UintegerValue(5));
  Simulator::Schedule(MilliSeconds(10), &ProcessingDelay::SubmitJob, pd, UintegerValue(6));
  Simulator::Schedule(MilliSeconds(24), &ProcessingDelayTest_expectCanStartImmediately, true);
  Simulator::Run();
  Simulator::Destroy();
  
  ASSERT_EQ(4U, ProcessingDelayTest_completed.size());
  EXPECT_EQ(1U, std::get<0>(ProcessingDelayTest_completed[0]));
  EXPECT_EQ(MilliSeconds(11), std::get<1>(ProcessingDelayTest_completed[0]));
  EXPECT_EQ(2U, std::get<0>(ProcessingDelayTest_completed[1]));
  EXPECT_EQ(MilliSeconds(15), std::get<1>(ProcessingDelayTest_completed[1]));
  EXPECT_EQ(5U, std::get<0>(ProcessingDelayTest_completed[2]));
  EXPECT_EQ(MilliSeconds(21), std::get<1>(ProcessingDelayTest_completed[2]));
  EXPECT_EQ(6U, std::get<0>(ProcessingDelayTest_completed[3]));
  EXPECT_EQ(MilliSeconds(25), std::get<1>(ProcessingDelayTest_completed[3]));

  ASSERT_EQ(2U, ProcessingDelayTest_dropped.size());
  EXPECT_EQ(3U, ProcessingDelayTest_dropped[0]);
  EXPECT_EQ(4U, ProcessingDelayTest_dropped[1]);
  
  EXPECT_LT(ProcessingDelayTest_expectCanStartImmediately_fail_time, MilliSeconds(0));
}

};//namespace ns3
