#ifndef NDNFD_STRATEGY_LAYER_H_
#define NDNFD_STRATEGY_LAYER_H_
#include "strategy.h"
#include "core/internal_client_handler.h"
namespace ndnfd {

// A StrategyLayer represents a forwarding strategy.
class StrategyLayer : public StrategyBase {
 public:
  StrategyLayer(void) {}
  virtual void Init(void);
  virtual ~StrategyLayer(void) {}
  Ptr<CcndStrategyInterface> ccnd_strategy_interface(void) const { return this->ccnd_strategy_interface_; }

  // -------- management --------
  
  // SetStrategy sets the strategy for a namespace.
  void SetStrategy(Ptr<const Name> prefix, StrategyType type) { assert(false); }//TODO impl: seek NPE, set strategy, recreate NpeExtra if strategy changed

  // -------- message entrypoint --------
  
  // OnInterest processes an incoming Interest.
  void OnInterest(Ptr<const InterestMessage> interest);
  
  // OnContent processes an incoming ContentObject.
  void OnContent(Ptr<const ContentObjectMessage> co);
  
  // TODO impl Content processing and remove WillSatisfyPendingInterest & DidSatisfyPendingInterests
  // WillSatisfyPendingInterest is invoked when a PIT entry is satisfied.
  void WillSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const Message> co, int pending_downstreams);
  // DidSatisfyPendingInterests is invoked after some PIT entries are satisfied in npe.
  // If matching_suffix is zero, some PIT entries are matched on this npe;
  // if matching_suffix is positive, some PIT entries are matched on a child of this npe;
  // if matching_suffix is negative, no PIT entry has been matched.
  void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix);

  // OnNack processes an incoming Nack.
  void OnNack(Ptr<const NackMessage> nack);

  // -------- table callbacks --------

  // NewNpeExtra adds extra information for root of a namespace.
  void NewNpeExtra(Ptr<NamePrefixEntry> npe);
  // InheritNpeExtra inherits extra information from parent node.
  void InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent);
  // FinalizeNpeExtra deletes extra information on npe.
  void FinalizeNpeExtra(Ptr<NamePrefixEntry> npe);
  
  // DidAddFibEntry is invoked when a FIB entry is created.
  void DidAddFibEntry(Ptr<ForwardingEntry> forw);
  // DidAddFibEntry is invoked when a FIB entry is deleted.
  void WillRemoveFibEntry(Ptr<ForwardingEntry> forw);//TODO invoke this

 private:
  Ptr<CcndStrategyInterface> ccnd_strategy_interface_;
  Ptr<Strategy> the_strategy_;//TODO remove this and get strategy from npe
  
  // FindStrategy returns a Strategy responsible for Name.
  Ptr<Strategy> FindStrategy(Ptr<const Name> name);
  Ptr<Strategy> FindStrategy(Ptr<Name> name) { return this->FindStrategy(const_cast<const Name*>(PeekPointer(name))); }
  Ptr<Strategy> FindStrategy(Ptr<const NamePrefixEntry> npe);
  Ptr<Strategy> FindStrategy(Ptr<NamePrefixEntry> npe) { return this->FindStrategy(const_cast<const NamePrefixEntry*>(PeekPointer(npe))); }
  
  DISALLOW_COPY_AND_ASSIGN(StrategyLayer);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_LAYER_H
