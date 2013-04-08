#ifndef NDNFD_STRATEGY_STRATEGY_H_
#define NDNFD_STRATEGY_STRATEGY_H_
#include "ccnd_interface.h"
#include "message/interest.h"
#include "message/contentobject.h"
#include "core/nameprefix_table.h"
namespace ndnfd {

// A Strategy represents a forwarding strategy.
class Strategy : public Element {
 public:
  Strategy(void) {}
  virtual void Init(void);
  virtual ~Strategy(void) {}
  Ptr<CcndStrategyInterface> ccnd_strategy_interface(void) const { return this->ccnd_strategy_interface_; }

  // -------- Interest processing --------

  // OnInterest processes an incoming Interest.
  // (same as ccnd process_incoming_interest)
  virtual void OnInterest(Ptr<InterestMessage> interest) {}//currently unused
  
  // SatisfyPendingInterestsOnFace satisfies all pending Interests on downstream.
  // It's invoked when incoming Interest is satisfied in CS.
  // (same as ccnd match_interests with face=downstream)
  virtual void SatisfyPendingInterestsOnFace(Ptr<const ContentObjectMessage> content, FaceId downstream) {}//currently unused
  
  // PropagateInterest propagates an Interest.
  // It's known that interest cannot be satisfied in CS.
  // (same as ccnd propagate_interest)
  virtual void PropagateInterest(Ptr<InterestMessage> interest, Ptr<NamePrefixEntry> npe);
  
  // LookupOutbounds returns a list of upstreams for an Interest,
  // usually from FIB.
  virtual std::unordered_set<FaceId> LookupOutbounds(Ptr<PitEntry> ie, Ptr<InterestMessage> interest);

  // PropagateNewInterest propagates the first Interest that causes creation of PIT entry.
  // Possible upstreams is populated in ie->pfl with CCND_PFI_UPSTREAM flag.
  // (same as ccnd strategy_callout CCNST_FIRST)
  virtual void PropagateNewInterest(Ptr<PitEntry> ie);

  // DoPropagate is invoked when
  // * a similar Interest is received from a new/expired downstream
  // * a downstream expires
  // * an upstream expires
  // It returns the delay until next time it should be called.
  // It should continue propagating until there's no more unexpired upstream,
  // in that case it should call DidExhaustForwardingOptions or WillEraseTimedOutPendingInterest.
  // (same as ccnd do_propagate)
  virtual std::chrono::microseconds DoPropagate(Ptr<PitEntry> ie);
  
  // DidnotArriveOnBestFace is invoked when ContentObject didn't arrived on best face
  // in predicted time.
  // (same as ccnd strategy_callout CCNST_TIMER)
  virtual void DidnotArriveOnBestFace(Ptr<PitEntry> ie);
  
  // WillEraseTimedOutPendingInterest is invoked when there's no more pending downstreams
  // and no more unexpired upstreams.
  // (same as ccnd strategy_callout CCNST_TIMEOUT)
  virtual void WillEraseTimedOutPendingInterest(Ptr<PitEntry> ie);

  // -------- ContentObject processing --------

  // OnContentObject processes an incoming ContentObject.
  // (same as ccnd process_incoming_content)
  virtual void OnContentObject(Ptr<ContentObjectMessage> content) { assert(false); }//currently unused
  
  // SatisfyPendingInterests satisfies pending Interests in npe that match content.
  // WillSatisfyPendingInterest is called before satisfying each Interest.
  // It returns downstream => number of Interests satisfied on that downtream; the caller should send content to them.
  // (same as ccnd match_interests with face=null)
  virtual std::unordered_map<FaceId,int> SatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const ContentObjectMessage> content) { return std::unordered_map<FaceId,int>(); }//currently unused
  
  // WillSatisfyPendingInterest is invoked when a PIT entry is satisfied.
  // (same as ccnd strategy_callout CCNST_SATISFIED)
  virtual void WillSatisfyPendingInterest(Ptr<PitEntry> ie, Ptr<const Message> co);
  // DidSatisfyPendingInterests is invoked after some PIT entries are satisfied in npe.
  // If matching_suffix is zero, some PIT entries are matched on this npe;
  // if matching_suffix is positive, some PIT entries are matched on a child of this npe;
  // if matching_suffix is negative, no PIT entry has been matched.
  // (similar to ccnd note_content_from but this is called for all shorter prefixes)
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix);
  
  // -------- face callbacks --------
  virtual void AddFace(FaceId face) {}//currently unused
  virtual void RemoveFace(FaceId face) {}//currently unused
  
  // -------- core table callbacks --------
  
  virtual void FinalizeNpeExtra(void* extra) {}
  
  // DidAddFibEntry is invoked when a FIB entry is created.
  // (same as update_npe_children)
  virtual void DidAddFibEntry(Ptr<ForwardingEntry> forw);
  
  virtual void WillRemoveFibEntry(Ptr<ForwardingEntry> forw) {}//currently unused

 protected:
  // -------- Interest service --------
  
  // PopulateOutbounds creates or updates a set of upstreams, and set their expiry time to zero.
  void PopulateOutbounds(Ptr<PitEntry> ie, const std::unordered_set<FaceId>& outbounds);
  
  // SendInterest sends an Interest to upstream on behalf of downstram.
  void SendInterest(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, Ptr<PitUpstreamRecord> upstream);
  
  // -------- ContentObject service --------

  // SendContentObject sends a ContentObject to downstream.
  // co is placed into the face send queue if there's no duplicate.
  void SendContentObject(FaceId downstream, content_entry* content) { assert(false); }//currently unused

 private:
  Ptr<CcndStrategyInterface> ccnd_strategy_interface_;

  DISALLOW_COPY_AND_ASSIGN(Strategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGY_H
