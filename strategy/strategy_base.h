#ifndef NDNFD_STRATEGY_STRATEGYBASE_H_
#define NDNFD_STRATEGY_STRATEGYBASE_H_
#include "ccnd_interface.h"
#include "message/interest.h"
#include "message/contentobject.h"
#include "message/nack.h"
#include "core/nameprefix_table.h"
namespace ndnfd {

// StrategyBase is the base class of all forwarding strategies, and the StrategyLayer.
// It provides building blocks for strategies.
class StrategyBase : public Element {
 public:
  StrategyBase(void) {}
  virtual ~StrategyBase(void) {}
  
 protected:
  // -------- sending --------
  
  // SendInterest sends an Interest to upstream on behalf of downstram.
  virtual void SendInterest(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, Ptr<PitUpstreamRecord> upstream);
  
  // SendContentObject sends a ContentObject to downstream.
  // co is placed into the face send queue if there's no duplicate.
  // TODO substitute content_entry* with a C++ type
  virtual void SendContent(FaceId downstream, content_entry* content) {}//TODO impl
  
  // SendNack sends a Nack to downstream.
  virtual void SendNack(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, NackCode code) {}//TODO impl
  virtual void SendNack(Ptr<InterestMessage> interest, NackCode code) {}//TODO impl
  
  // -------- FIB/PIT --------

  // LookupOutbounds returns a list of upstreams for an Interest,
  // StrategyBase impl queries FIB.
  virtual std::unordered_set<FaceId> LookupOutbounds(Ptr<PitEntry> ie, Ptr<const InterestMessage> interest);

  // PopulateOutbounds creates or updates a set of upstreams, and set their expiry time to zero.
  void PopulateOutbounds(Ptr<PitEntry> ie, const std::unordered_set<FaceId>& outbounds);
  
  // SatisfyPendingInterestsOnFace satisfies all pending Interests on downstream.
  // It's invoked when incoming Interest is satisfied in CS.
  // It should not send the Content.
  // (same as ccnd match_interests with face=downstream)
  virtual void SatisfyPendingInterestsOnFace(content_entry* content, FaceId downstream) { assert(false); }//TODO impl
  
  // -------- CS --------
  
  // MatchContentStore finds a Content matching the Interest,
  // or returns null if no matching or Interest selectors don't allow matching
  // (AOK_CS is missing, or Content is stale but AOK_STALE is missing).
  // It should not send the Content.
  // TODO substitute content_entry* with a C++ type
  virtual content_entry* MatchContentStore(Ptr<InterestMessage> interest) { return nullptr; }//TODO impl
  
  // EnrollContent adds a Content to the ContentStore,
  // or refreshes an existing entry if it's stale.
  // It returns null when there's a failure.
  // TODO substitute content_entry* with a C++ type
  virtual content_entry* EnrollContent(Ptr<ContentObjectMessage> co) { return nullptr; }//TODO impl
  
  // RemoveContent removes a Content from the ContentStore.
  // TODO substitute content_entry* with a C++ type
  virtual void RemoveContent(content_entry* content) {}//TODO impl

 private:
  DISALLOW_COPY_AND_ASSIGN(StrategyBase);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_STRATEGYBASE_H
