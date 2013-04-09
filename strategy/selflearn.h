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
  virtual void DidnotArriveOnBestFace(Ptr<PitEntry> ie) { assert(false); }
  virtual void DidSatisfyPendingInterests(Ptr<NamePrefixEntry> npe, Ptr<const Message> co, int matching_suffix);
  virtual void FinalizeNpeExtra(void* extra);

 protected:
  // SendInterest sends Interest, and calls DidnotArriveOnFace if no ContentObject comes back in predict time.
  virtual void SendInterest(Ptr<PitEntry> ie, Ptr<PitDownstreamRecord> downstream, Ptr<PitUpstreamRecord> upstream);

 private:
  struct PredictRecord {
    std::chrono::microseconds time_;
    int rank_;
    std::chrono::microseconds accum_;
  };
  struct NpeExtra {
    std::unordered_map<FaceId,PredictRecord> predicts_;
  };
  
  static std::chrono::microseconds initial_prediction(void) { return std::chrono::microseconds(15000); }
  // GetExtra returns extra field for npe,
  // copy from parent or create for root if it does not exist.
  NpeExtra* GetExtra(Ptr<NamePrefixEntry> npe);
  // RankPredicts sorts the predicts set by increasing prediction.
  void RankPredicts(NpeExtra* extra);

  void DidnotArriveOnFace(Ptr<PitEntry> ie, FaceId face);
  
  // StartFlood adds new upstreams to broadcast the Interests
  void StartFlood(Ptr<PitEntry> ie);

  DISALLOW_COPY_AND_ASSIGN(SelfLearnStrategy);
};

};//namespace ndnfd
#endif//NDNFD_STRATEGY_SELFLEARN_H
