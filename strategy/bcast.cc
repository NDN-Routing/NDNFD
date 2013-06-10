#include "bcast.h"
namespace ndnfd {

void BcastStrategy::PropagateNewInterest(Ptr<PitEntry> ie) {
  // Do nothing here, so DoPropagate would zero defer times on all upstreams,
  // and immediately send out Interests.
}

};//namespace ndnfd
