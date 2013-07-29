#ifndef NDNFD_UTIL_RTT_H_
#define NDNFD_UTIL_RTT_H_
#include <chrono>
#include "core/element.h"
namespace ndnfd {

// An RttEstimator implements the Mean-Deviation RTT estimator.
class RttEstimator {
 public:
  typedef std::chrono::microseconds time;
  static constexpr time initial_rtt(void) { return std::chrono::duration_cast<time>(std::chrono::seconds(1)); }
  
  RttEstimator(uint16_t max_multiplier = 16, time min_rto = time(10), double gain = 0.1);
  virtual ~RttEstimator(void) {}
  RttEstimator(const RttEstimator& other) { this->operator=(other); }
  RttEstimator& operator=(const RttEstimator& other);
  
  void Measurement(time measure);
  void IncrementMultiplier(void);
  void IncreaseMultiplier(void);

  time RetransmitTimeout(void) const;
  
 private:
  uint16_t max_multiplier_;
  double min_rto_;
  
  double rtt_;
  double gain_;
  double variance_;
  uint16_t multiplier_;
  uint32_t n_samples_;
};


};//namespace ndnfd
#endif//NDNFD_UTIL_RTT_H_
