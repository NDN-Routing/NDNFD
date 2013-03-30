#ifndef NDNFD_STRATEGY_STRATEGY_H_
#define NDNFD_STRATEGY_STRATEGY_H_
extern "C" {
#include "ccnd/ccnd_private.h"
}
#include "face/faceid.h"
#include "message/interest.h"
#include "message/contentobject.h"
#include "core/nameprefix_table.h"
namespace ndnfd {


// A Strategy represents a forwarding strategy.
// This is not currently used.
// The design is taken from ndnSIM, but it seems insufficient.
class Strategy : public Element {
 public:
  Strategy(void);
  virtual ~Strategy(void);

#define CURRENTLY_UNUSED =delete

  // -------- Interest processing --------

  // OnInterest processes an incoming Interest.  
  virtual void OnInterest(Ptr<InterestMessage> interest) CURRENTLY_UNUSED;//process_incoming_interest
  
  // SatisfyPendingInterestsOnFace satisfies all pending Interests on downstream.
  // It's invoked when incoming Interest is satisfied in CS.
  // (same as ccnd match_interests with face=downstream)
  virtual void SatisfyPendingInterestsOnFace(Ptr<const ContentObjectMessage> content, FaceId downstream) CURRENTLY_UNUSED;

  // PropagateInterest propagates an Interest.
  // It's known that interest cannot be satisfied in CS.
  // (same as ccnd propagate_interest)
  virtual void PropagateInterest(Ptr<InterestMessage> interest, Ptr<NamePrefixEntry> npe);

  // FailedToFindFibEntry is invoked if there's no FIB entry for Interest.
  // It can return false to indicate propagation should fail.
  // It can try other options, and return true if Interest is propagated.
  virtual bool FailedToFindFibEntry(Ptr<InterestMessage> interest, Ptr<PitEntry> ie) { return false; }

  // PropagateNewInterest propagates the first Interest that causes creation of PIT entry.
  // Possible upstreams is populated in ie->pfl with CCND_PFI_UPSTREAM flag.
  // (same as ccnd strategy_callout CCNST_FIRST)
  virtual void PropagateNewInterest(Ptr<PitEntry> ie);
  
  // DoPropagate is invoked when
  // * a similar Interest is received from another downstream
  // * a downstream times out
  // * an upstream expires
  // It returns the next time it should be called.
  // It should continue propagating until there's no more unexpired upstream,
  // in that case it should call DidExhaustForwardingOptions or WillEraseTimedOutPendingInterest.
  // (same as ccnd do_propagate)
  virtual std::chrono::microseconds DoPropagate(Ptr<PitEntry> ie);
  
  // DidExhaustForwardingOptions is invoked when there are pending downstreams,
  // but no more unexpired upstreams.
  virtual void DidExhaustForwardingOptions(Ptr<PitEntry> ie) {}
  
  // WillEraseTimedOutPendingInterest is invoked when there's no more pending downstreams
  // and no more unexpired upstreams.
  // (same as ccnd strategy_callout CCNST_TIMEOUT)
  virtual void WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie) {}

  // -------- ContentObject processing --------

  // OnContentObject processes an incoming ContentObject.
  // (same as ccnd process_incoming_content)
  virtual void OnContentObject(Ptr<ContentObjectMessage> content) CURRENTLY_UNUSED;
  
  // SatisfyPendingInterests satisfies pending Interests in npe that match content.
  // If downstream is FaceId_none, WillSatisfyPendingInterest is called before satisfying each Interest.
  // It returns downstream => number of Interests satisfied on that downtream; the caller should send content to them.
  // (same as ccnd match_interests with face=null)
  virtual std::unordered_map<FaceId,int> SatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const ContentObjectMessage> content) CURRENTLY_UNUSED;
  
  // WillSatisfyPendingInterest is invoked when a PIT entry is satisfied.
  // (same as ccnd strategy_callout CCNST_SATISFIED)
  virtual void WillSatisfyPendingInterest(Ptr<PitEntry> ie, FaceId upstream, FaceId downstream) {}
  
  // -------- face callbacks --------
  virtual void AddFace(FaceId face) CURRENTLY_UNUSED;
  virtual void RemoveFace(FaceId face) CURRENTLY_UNUSED;
  
  // -------- FIB callbacks --------
  virtual void DidAddFibEntry(Ptr<ForwardingEntry> forw) CURRENTLY_UNUSED;
  virtual void WillRemoveFibEntry(Ptr<ForwardingEntry> forw) CURRENTLY_UNUSED;

 private:
  DISALLOW_COPY_AND_ASSIGN(Strategy);
#undef CURRENTLY_UNUSED
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGY_H
