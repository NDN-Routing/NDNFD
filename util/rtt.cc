#include "util/rtt.h"
#include <cmath>
#include <algorithm>
namespace ndnfd {

// reference: ns3::RttMeanDeviation

RttEstimator::RttEstimator(uint16_t max_multiplier, time min_rto, double gain)
    : max_multiplier_(max_multiplier), min_rto_(min_rto.count()),
      rtt_(RttEstimator::initial_rtt().count()), gain_(gain), variance_(0),
      multiplier_(1), n_samples_(0) {}

void RttEstimator::Measurement(time measure) {
  double m = static_cast<double>(measure.count());
  if (this->n_samples_ > 0) {
    double err = m - this->rtt_;
    double gerr = err * this->gain_;
    this->rtt_ += gerr;
    double difference = abs(err) - this->variance_;
    this->variance_ += difference * this->gain_;
  } else {
    this->rtt_ = m;
    this->variance_ = m;
  }
  ++this->n_samples_;
  this->multiplier_ = 1;
}

void RttEstimator::IncreaseMultiplier(void) {
  this->multiplier_ = std::min(static_cast<uint16_t>(this->multiplier_ * 2), this->max_multiplier_);
}

RttEstimator::time RttEstimator::RetransmitTimeout(void) const {
  double rto = std::max(this->min_rto_, this->rtt_ + 4 * this->variance_);
  rto *= this->multiplier_;
  return time(static_cast<time::rep>(rto));
}

};//namespace ndnfd
