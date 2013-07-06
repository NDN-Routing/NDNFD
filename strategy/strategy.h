#ifndef NDNFD_STRATEGY_STRATEGY_H_
#define NDNFD_STRATEGY_STRATEGY_H_
#include "strategy_base.h"
#include "strategy_type.h"
namespace ndnfd {

// A Strategy represents a forwarding strategy.
class Strategy : public StrategyBase {
 public:
  virtual ~Strategy(void) {}
  virtual StrategyType strategy_type(void) const =0;

  // -------- Interest processing --------

  // OnInterest processes an incoming Interest.
  // This is triggered on the strategy responsible for interest->name().
  virtual void OnInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe, Ptr<PitEntry> ie);
 
  // -------- ContentObject processing --------
  
  // DidSatisfyPendingInterest is invoked when a PIT entry is satisfied.
  // This is triggered on the strategy responsible for ie->name().
  virtual void DidSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int pending_downstreams) {}
  // DidReceiveContent is invoked when a ContentObject under npe arrived.
  //   matching_suffix = 0: some Interests are satisfied on this npe
  //   matching_suffix > 0: some Interests are satisfied on child_npe, child_npe->n_comps() == npe->n_comps()+matching_suffix
  //   matching_suffix < 0: no Interest is satisfied so far
  // This is triggered on the strategy responsible for npe->name(), and is triggered multiple times towards root npe.
  virtual void DidReceiveContent(Ptr<NamePrefixEntry> npe, Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co, int matching_suffix) {}
  // DidReceiveUnsolicitedContent is invoked when an unsolicited ContentObject arrived.
  // This is triggered on the strategy responsible for ce->name(), and is triggered after DidReceiveContent.
  virtual void DidReceiveUnsolicitedContent(Ptr<const ContentEntry> ce, Ptr<const ContentObjectMessage> co) {}
  
  // -------- Nack processing --------
  
  // OnNack processes an incoming Nack.
  // This is triggered on the strategy responsible for nack->interest()->name().
  virtual void OnNack(Ptr<const NackMessage> nack);
  
  // -------- core table callbacks --------
  // NewNpeExtra adds extra information for root of a namespace.
  virtual void NewNpeExtra(Ptr<NamePrefixEntry> npe) {}
  // InheritNpeExtra inherits extra information from parent node.
  virtual void InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent) {}
  // FinalizeNpeExtra deletes extra information on npe.
  virtual void FinalizeNpeExtra(Ptr<NamePrefixEntry> npe) {}
  
  // DidAddFibEntry is invoked when a FIB entry is created.
  virtual void DidAddFibEntry(Ptr<ForwardingEntry> forw);
  
  virtual void WillRemoveFibEntry(Ptr<ForwardingEntry> forw) {}//currently unused

 protected:
  Strategy(void) {}

  // -------- Interest processing --------
 
  // PropagateInterest propagates an Interest that cannot be satisfied in ContentStore.
  virtual void PropagateInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe);
  
  // PropagateNewInterest propagates the first Interest that causes creation of PIT entry.
  // Possible upstreams is populated as PitUpstreamRecords.
  virtual void PropagateNewInterest(Ptr<PitEntry> ie) =0;

  // SchedulePropagate schedules the start of a propagation plan.
  void SchedulePropagate(Ptr<PitEntry> ie, std::chrono::microseconds defer);

  // DoPropagate executes a propagation plan.
  // It returns the delay until next time it should be called.
  std::chrono::microseconds DoPropagate(Ptr<PitEntry> ie);
  
  // WillEraseTimedOutPendingInterest is invoked when there's no more pending downstreams
  // and no more unexpired upstreams.
  virtual void WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie);

 private:
  DISALLOW_COPY_AND_ASSIGN(Strategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGY_H
