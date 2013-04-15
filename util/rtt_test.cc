#include "rtt.h"
#include "core/element_testh.h"
#include "gtest/gtest.h"
namespace ndnfd {

TEST(UtilTest, RttEstimator) {
  Ptr<RttEstimator> rtt = NewTestElement<RttEstimator>();
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt->RetransmitTimeout().count()));
  rtt->Measurement(RttEstimator::time(10000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt->RetransmitTimeout().count()));
  rtt->Measurement(RttEstimator::time(11000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt->RetransmitTimeout().count()));
  rtt->Measurement(RttEstimator::time(10000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt->RetransmitTimeout().count()));
  rtt->IncreaseMultiplier();
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt->RetransmitTimeout().count()));
  rtt->Measurement(RttEstimator::time(10000));
  printf("%" PRIuMAX "\n", static_cast<uintmax_t>(rtt->RetransmitTimeout().count()));
}

};//namespace ndnfd
