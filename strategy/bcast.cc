#include "bcast.h"
namespace ndnfd {

void BcastStrategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  // Strategy::PropagateNewInterest sends Interest to the best face,
  // and sets defer times on other possible faces.
  // To use broadcast, do nothing here, so DoPropagate would see
  // defer times to be all zero, and immediately send out Interests.
}

};//namespace ndnfd
