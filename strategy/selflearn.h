#ifndef NDNFD_STRATEGY_SELFLEARN_H_
#define NDNFD_STRATEGY_SELFLEARN_H_
#include "strategy.h"
namespace ndnfd {

// SelfLearnStrategy broadcast the first Interest,
// learns about where is the producer, send subsequent Interests to that face only,
// until there's a timeout which would trigger another broadcast.
class SelfLearnStrategy : public Strategy {
 public:
  SelfLearnStrategy(void) {}
  virtual ~SelfLearnStrategy(void) {}

  virtual std::unordered_set<FaceId> LookupOutbounds(Ptr<PitEntry> ie, Ptr<InterestMessage> interest);
  virtual void PropagateNewInterest(Ptr<PitEntry> ie);
  virtual void DidnotArriveOnBestFace(Ptr<PitEntry> ie);
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co);

 private:
  // StartFlood adds new upstreams to broadcast the Interests
  void StartFlood(Ptr<PitEntry> ie);

  DISALLOW_COPY_AND_ASSIGN(SelfLearnStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_SELFLEARN_H
