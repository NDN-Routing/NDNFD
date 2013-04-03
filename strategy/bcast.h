#ifndef NDNFD_STRATEGY_BCAST_H_
#define NDNFD_STRATEGY_BCAST_H_
#include "strategy.h"
namespace ndnfd {

// BcastStrategy forwards an Interest to every face returned from FIB lookup,
// except the incoming face.
class BcastStrategy : public Strategy {
 public:
  BcastStrategy(void) {}
  virtual ~BcastStrategy(void) {}

  virtual void PropagateNewInterest(Ptr<PitEntry> ie);

 private:
  DISALLOW_COPY_AND_ASSIGN(BcastStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_BCAST_H
