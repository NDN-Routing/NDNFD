#include "rtt.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(UtilTest, RttEstimator) {
  RttEstimator rtt;
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt.RetransmitTimeout().count()));
  rtt.Measurement(RttEstimator::time(10000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt.RetransmitTimeout().count()));
  rtt.Measurement(RttEstimator::time(11000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt.RetransmitTimeout().count()));
  rtt.Measurement(RttEstimator::time(10000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt.RetransmitTimeout().count()));
  rtt.IncreaseMultiplier();
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt.RetransmitTimeout().count()));
  rtt.Measurement(RttEstimator::time(10000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt.RetransmitTimeout().count()));

  RttEstimator rtt2(rtt);
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt2.RetransmitTimeout().count()));
  RttEstimator rtt3; rtt3 = rtt;
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt3.RetransmitTimeout().count()));
}

};//namespace ndnfd
