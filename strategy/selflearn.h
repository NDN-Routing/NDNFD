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

  virtual std::unordered_set<FaceId> LookupOutbounds(Ptr<NamePrefixEntry> npe, Ptr<InterestMessage> interest, Ptr<PitEntry> ie);
  virtual bool DidExhaustForwardingOptions(Ptr<PitEntry> ie);
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co);

 private:
  DISALLOW_COPY_AND_ASSIGN(SelfLearnStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_SELFLEARN_H
