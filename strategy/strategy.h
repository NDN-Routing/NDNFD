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
  virtual void OnInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe, Ptr<PitEntry> ie);
 
  // -------- ContentObject processing --------

  // OnContent processes an incoming Content.
  // (hand over to ccnd process_incoming_content)
  virtual void OnContent(Ptr<const ContentObjectMessage> co);
  
  // SatisfyPendingInterests satisfies pending Interests in npe that match content.
  // WillSatisfyPendingInterest is called before satisfying each Interest.
  // It returns downstream => number of Interests satisfied on that downtream; the caller should send content to them.
  // (same as ccnd match_interests with face=null)
  virtual std::unordered_map<FaceId,int> SatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const ContentObjectMessage> co) { return std::unordered_map<FaceId,int>(); }//TODO invoke & impl
  
  // WillSatisfyPendingInterest is invoked when a PIT entry is satisfied.
  // (same as ccnd strategy_callout CCNST_SATISFIED)
  virtual void WillSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const Message> co, int pending_downstreams);
  // DidSatisfyPendingInterests is invoked after some PIT entries are satisfied in npe.
  // If matching_suffix is zero, some PIT entries are matched on this npe;
  // if matching_suffix is positive, some PIT entries are matched on a child of this npe;
  // if matching_suffix is negative, no PIT entry has been matched.
  // (similar to ccnd note_content_from but this is called for all shorter prefixes)
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix) {}
  
  // -------- Nack processing --------
  
  // OnNack processes an incoming Nack.
  virtual void OnNack(Ptr<const NackMessage> nack);
  
  // -------- face callbacks --------
  virtual void AddFace(FaceId face) {}//currently unused
  virtual void RemoveFace(FaceId face) {}//currently unused
  
  // -------- core table callbacks --------
  // NewNpeExtra adds extra information for root of a namespace.
  virtual void NewNpeExtra(Ptr<NamePrefixEntry> npe) {}
  // InheritNpeExtra inherits extra information from parent node.
  virtual void InheritNpeExtra(Ptr<NamePrefixEntry> npe, Ptr<const NamePrefixEntry> parent) {}
  // FinalizeNpeExtra deletes extra information on npe.
  virtual void FinalizeNpeExtra(Ptr<NamePrefixEntry> npe) {}
  
  // DidAddFibEntry is invoked when a FIB entry is created.
  // (same as update_npe_children)
  virtual void DidAddFibEntry(Ptr<ForwardingEntry> forw);
  
  virtual void WillRemoveFibEntry(Ptr<ForwardingEntry> forw) {}//currently unused

 protected:
  Strategy(void) {}

  // -------- Interest processing --------
 
  // PropagateInterest propagates an Interest.
  // It's known that interest cannot be satisfied in CS.
  // (same as ccnd propagate_interest)
  virtual void PropagateInterest(Ptr<const InterestMessage> interest, Ptr<NamePrefixEntry> npe);
  
  // PropagateNewInterest propagates the first Interest that causes creation of PIT entry.
  // Possible upstreams is populated as PitUpstreamRecords.
  virtual void PropagateNewInterest(Ptr<PitEntry> ie) =0;

  // SchedulePropagate schedules the start of a propagation plan.
  void SchedulePropagate(Ptr<PitEntry> ie, std::chrono::microseconds defer);

  // DoPropagate is invoked when
  // * a similar Interest is received from a new/expired downstream
  // * a downstream expires
  // * an upstream expires
  // It returns the delay until next time it should be called.
  // It should continue propagating until there's no more unexpired upstream,
  // in that case it should call DidExhaustForwardingOptions or WillEraseTimedOutPendingInterest.
  // (same as ccnd do_propagate)
  std::chrono::microseconds DoPropagate(Ptr<PitEntry> ie);
  
  // WillEraseTimedOutPendingInterest is invoked when there's no more pending downstreams
  // and no more unexpired upstreams.
  // (same as ccnd strategy_callout CCNST_TIMEOUT)
  virtual void WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie);



 private:
  DISALLOW_COPY_AND_ASSIGN(Strategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGY_H
