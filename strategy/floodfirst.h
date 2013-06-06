#ifndef NDNFD_STRATEGY_FLOODFIRST_H_
#define NDNFD_STRATEGY_FLOODFIRST_H_
#include "strategy.h"
namespace ndnfd {

// FloodFirstStrategy is a simpler self-learning strategy without RttEstimator.
// 1. To propagate an Interest with no FIB match, flood to all multicast faces.
// 2. When a ContentObject arrives, create a forwarding entry at fib_prefix_comps() components;
//    the forwarding entry is valid for fib_entry_expires() seconds, and refreshed on every CO.
// 3. In other situations, behave same as ccnd default strategy.
class FloodFirstStrategy : public Strategy {
 public:
  FloodFirstStrategy(void) {}
  virtual ~FloodFirstStrategy(void) {}
  uint16_t fib_prefix_comps(void) const { return 1; }
  std::chrono::seconds fib_entry_expires(void) const { return std::chrono::seconds(2); }

  virtual void PropagateNewInterest(Ptr<PitEntry> ie);
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix);

 private:
  DISALLOW_COPY_AND_ASSIGN(FloodFirstStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_FLOODFIRST_H
